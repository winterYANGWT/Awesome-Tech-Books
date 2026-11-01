#!/usr/bin/env python3
"""shorty-operator：把 ShortyBackup 协调成 pg_dump Job。

这是一个刻意写小的控制器：每 2 秒列一次 CR，reconcile 是幂等的；
生产实现会用 client-go 的 informer 和限速工作队列，reconcile 的内容不变。
"""
import logging
import os
import signal
import socket
import sys
import time
from datetime import datetime, timezone

from kubernetes import client, config
from kubernetes.client.rest import ApiException

GROUP, VERSION, PLURAL = "shorty.example.com", "v1alpha1", "shortybackups"
NAMESPACE = os.environ.get("WATCH_NAMESPACE", "shorty")
JOB_IMAGE = os.environ.get("BACKUP_IMAGE", "postgres:18.6-alpine")
FINALIZER = "shorty.example.com/backup-cleanup"
LEASE, INTERVAL = "shorty-operator-lock", 2

logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
log = logging.getLogger("shorty-operator")


def now():
    return datetime.now(timezone.utc).isoformat().replace("+00:00", "Z")


def condition(old, status, reason, message):
    previous = next((c for c in old if c.get("type") == "Ready"), {})
    changed = previous.get("status") != status or previous.get("reason") != reason
    return {"type": "Ready", "status": status, "reason": reason, "message": message,
            "lastTransitionTime": now() if changed else previous.get("lastTransitionTime", now())}


class LeaderElector:
    """用 coordination.k8s.io/v1 Lease 选主；同一时刻只有一个副本 reconcile。"""

    def __init__(self, api):
        self.api, self.identity, self.is_leader = api, socket.gethostname(), False
        self.standby_logged = False

    def acquire(self):
        stamp = datetime.now(timezone.utc)
        try:
            lease = self.api.read_namespaced_lease(LEASE, NAMESPACE)
        except ApiException as exc:
            if exc.status != 404:
                log.warning("read lease failed: %s", exc)
                self.is_leader = False
                return False
            body = {"metadata": {"name": LEASE, "namespace": NAMESPACE},
                    "spec": {"holderIdentity": self.identity, "leaseDurationSeconds": 15,
                             "acquireTime": stamp, "renewTime": stamp}}
            try:
                self.api.create_namespaced_lease(NAMESPACE, body)
            except ApiException as exc:
                log.warning("create lease failed: %s", exc)
                self.is_leader = False
                return False
            self.became_leader()
            return True
        spec = lease.spec
        expired = spec.renew_time is None or (stamp - spec.renew_time).total_seconds() > (spec.lease_duration_seconds or 15)
        if spec.holder_identity != self.identity and not expired:
            self.is_leader = False
            if not self.standby_logged:
                log.info("standby: leader is %s", spec.holder_identity)
                self.standby_logged = True
            return False
        patch = {"spec": {"holderIdentity": self.identity, "leaseDurationSeconds": 15,
                          "acquireTime": spec.acquire_time if spec.holder_identity == self.identity else stamp,
                          "renewTime": stamp}}
        if spec.holder_identity != self.identity:
            patch["spec"]["leaseTransitions"] = (spec.lease_transitions or 0) + 1
        try:
            self.api.patch_namespaced_lease(LEASE, NAMESPACE, patch)
        except ApiException as exc:
            log.warning("renew lease failed: %s", exc)
            self.is_leader = False
            return False
        self.became_leader()
        return True

    def became_leader(self):
        if not self.is_leader:
            log.info("became leader %s", self.identity)
        self.is_leader, self.standby_logged = True, False


def owner_ref(cr):
    return {"apiVersion": f"{GROUP}/{VERSION}", "kind": "ShortyBackup", "name": cr["metadata"]["name"],
            "uid": cr["metadata"]["uid"], "controller": True, "blockOwnerDeletion": True}


def labels(name):
    return {"app.kubernetes.io/name": "shorty-backup", "app.kubernetes.io/instance": name,
            "app.kubernetes.io/part-of": "shorty"}


def backup_job(cr):
    name = cr["metadata"]["name"]
    script = ('set -eu\nexport PGPASSWORD="$(cat /etc/shorty/db_password)"\n'
              'echo "dumping database shorty"\n'
              'pg_dump -h postgres -U shorty -d shorty -f /backup/shorty.sql\n'
              'wc -c /backup/shorty.sql\n')
    return {
        "metadata": {"name": f"{name}-pgdump", "namespace": NAMESPACE, "labels": labels(name),
                     "ownerReferences": [owner_ref(cr)]},
        "spec": {"backoffLimit": 1, "template": {
            "metadata": {"labels": labels(name)},
            "spec": {
                "restartPolicy": "Never",
                "securityContext": {"runAsNonRoot": True, "runAsUser": 70, "runAsGroup": 70,
                                    "fsGroup": 70, "seccompProfile": {"type": "RuntimeDefault"}},
                "containers": [{
                    "name": "pgdump", "image": JOB_IMAGE, "imagePullPolicy": "IfNotPresent",
                    "command": ["/bin/sh", "-c"], "args": [script],
                    "securityContext": {"allowPrivilegeEscalation": False, "readOnlyRootFilesystem": True,
                                        "capabilities": {"drop": ["ALL"]}},
                    "volumeMounts": [{"name": "backup", "mountPath": "/backup"},
                                     {"name": "tmp", "mountPath": "/tmp"},
                                     {"name": "db-secret", "mountPath": "/etc/shorty", "readOnly": True}],
                    "resources": {"requests": {"cpu": "10m", "memory": "32Mi"},
                                  "limits": {"cpu": "200m", "memory": "128Mi"}}}],
                "volumes": [
                    {"name": "backup", "persistentVolumeClaim": {"claimName": f"{name}-data"}},
                    {"name": "tmp", "emptyDir": {}},
                    {"name": "db-secret", "secret": {"secretName": "shorty-db", "defaultMode": 0o444}}],
            }}}}


def backup_pvc(cr):
    name = cr["metadata"]["name"]
    return {"metadata": {"name": f"{name}-data", "namespace": NAMESPACE, "labels": labels(name),
                         "ownerReferences": [owner_ref(cr)]},
            "spec": {"accessModes": ["ReadWriteOnce"], "resources": {"requests": {"storage": "1Gi"}}}}


def set_status(api, cr, phase, reason, message, job=None):
    old = cr.get("status") or {}
    status = {"phase": phase, "observedGeneration": cr["metadata"]["generation"],
              "jobName": job or old.get("jobName"),
              "conditions": [condition(old.get("conditions") or [],
                                       "True" if phase == "Completed" else "False", reason, message)]}
    if phase == "Completed":
        status["lastBackupTime"] = old.get("lastBackupTime") if old.get("phase") == "Completed" else now()
    if status == old:
        return
    try:
        api.patch_namespaced_custom_object_status(GROUP, VERSION, NAMESPACE, PLURAL,
                                                  cr["metadata"]["name"], {"status": status})
    except ApiException as exc:
        if exc.status != 409:
            log.warning("update status %s failed: %s", cr["metadata"]["name"], exc)
    log.info("reconcile %s/%s generation=%s phase=%s", NAMESPACE, cr["metadata"]["name"],
             cr["metadata"]["generation"], phase)


def delete_owned(api, kind, name):
    try:
        if kind == "job":
            api.delete_namespaced_job(name, NAMESPACE, propagation_policy="Background")
        else:
            api.delete_namespaced_persistent_volume_claim(name, NAMESPACE)
    except ApiException as exc:
        if exc.status != 404:
            log.warning("delete %s %s failed: %s", kind, name, exc)
            return False
    return True


def cleanup(custom, batch, core, cr):
    name = cr["metadata"]["name"]
    if not delete_owned(batch, "job", f"{name}-pgdump") or not delete_owned(core, "pvc", f"{name}-data"):
        return
    finalizers = [f for f in cr["metadata"].get("finalizers", []) if f != FINALIZER]
    custom.patch_namespaced_custom_object(GROUP, VERSION, NAMESPACE, PLURAL, name,
                                         {"metadata": {"finalizers": finalizers}})
    log.info("cleanup %s: deleted job and pvc, removed finalizer", name)


def reconcile(custom, batch, core, cr):
    name = cr["metadata"]["name"]
    if cr["metadata"].get("deletionTimestamp"):
        cleanup(custom, batch, core, cr)
        return
    if FINALIZER not in cr["metadata"].get("finalizers", []):
        finalizers = cr["metadata"].get("finalizers", []) + [FINALIZER]
        custom.patch_namespaced_custom_object(GROUP, VERSION, NAMESPACE, PLURAL, name,
                                             {"metadata": {"finalizers": finalizers}})
        log.info("added finalizer to %s", name)
    try:
        core.read_namespaced_persistent_volume_claim(f"{name}-data", NAMESPACE)
    except ApiException as exc:
        if exc.status != 404:
            return set_status(custom, cr, "Failed", "PVCCreateFailed", str(exc)[:200])
        try:
            core.create_namespaced_persistent_volume_claim(NAMESPACE, backup_pvc(cr))
            log.info("created pvc %s-data", name)
        except ApiException as exc:
            log.error("create pvc %s-data failed: %s", name, exc)
            return set_status(custom, cr, "Failed", "PVCCreateFailed", str(exc)[:200])
    try:
        job = batch.read_namespaced_job(f"{name}-pgdump", NAMESPACE)
    except ApiException as exc:
        if exc.status != 404:
            return set_status(custom, cr, "Failed", "JobReadFailed", str(exc)[:200])
        try:
            batch.create_namespaced_job(NAMESPACE, backup_job(cr))
            log.info("created job %s-pgdump for %s", name, name)
        except ApiException as exc:
            log.error("create job %s-pgdump failed: %s", name, exc)
            return set_status(custom, cr, "Failed", "JobCreateFailed", str(exc)[:200])
        return set_status(custom, cr, "Running", "JobCreated", f"created job {name}-pgdump", f"{name}-pgdump")
    if job.status.succeeded:
        set_status(custom, cr, "Completed", "BackupCompleted", f"job {name}-pgdump completed", f"{name}-pgdump")
    elif job.status.failed:
        set_status(custom, cr, "Failed", "JobFailed", f"job {name}-pgdump failed", f"{name}-pgdump")
    else:
        set_status(custom, cr, "Running", "JobRunning", f"job {name}-pgdump is running", f"{name}-pgdump")


def main():
    config.load_incluster_config()
    custom, batch, core = client.CustomObjectsApi(), client.BatchV1Api(), client.CoreV1Api()
    elector = LeaderElector(client.CoordinationV1Api())
    log.info("shorty-operator started namespace=%s identity=%s", NAMESPACE, elector.identity)
    signal.signal(signal.SIGTERM, lambda *_: sys.exit(0))
    while True:
        if elector.acquire():
            try:
                for cr in custom.list_namespaced_custom_object(GROUP, VERSION, NAMESPACE, PLURAL)["items"]:
                    reconcile(custom, batch, core, cr)
            except ApiException as exc:
                log.warning("list shortybackups failed: %s", exc)
            except Exception:
                log.exception("reconcile loop failed")
        time.sleep(INTERVAL)


if __name__ == "__main__":
    main()
