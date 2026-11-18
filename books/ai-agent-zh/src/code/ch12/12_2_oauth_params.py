# mode: run
# llm: none
# expect: code_challenge
# expect: resource
# expect: iss
# expect: S256
"""本地假元数据走一遍 OAuth 2.1 授权：PRM → AS metadata → 授权 → 回调 → token。"""
import base64
import hashlib
import json
from urllib.parse import urlencode

RESOURCE = "https://books.example/mcp"
AS = "https://auth.example"
PRM_URL = "https://books.example/.well-known/oauth-protected-resource/mcp"
CLIENT_ID = "https://client.example/oauth/client.json"
REDIRECT = "http://127.0.0.1:8765/callback"
SCOPE = "orders:read refund:write"
STATE = "st-7f3a9c"
VERIFIER = "dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk"
CODE = "ac-4f2c8e"

PRM = {"resource": RESOURCE, "authorization_servers": [AS],
       "scopes_supported": ["orders:read", "refund:write"]}
AS_METADATA = {
    "issuer": AS,
    "authorization_endpoint": AS + "/authorize",
    "token_endpoint": AS + "/token",
    "code_challenge_methods_supported": ["S256"],
    "client_id_metadata_document_supported": True,
    "authorization_response_iss_parameter_supported": True,
}
WWW_AUTH = (f'Bearer resource_metadata="{PRM_URL}", scope="{SCOPE}"')


def b64url(raw: bytes) -> str:
    return base64.urlsafe_b64encode(raw).decode().rstrip("=")


def main():
    print("[1] 未带 token 的请求，server 回 401")
    print("WWW-Authenticate:", WWW_AUTH)
    print("[2] Protected Resource Metadata（RFC 9728）")
    print(json.dumps(PRM, ensure_ascii=False, indent=1))
    print("[3] Authorization Server Metadata（RFC 8414）")
    print(json.dumps(AS_METADATA, ensure_ascii=False, indent=1))
    print("[4] 选择 client_id：有预注册先用，否则 CIMD，最后 DCR")
    print("client_id:", CLIENT_ID)

    # ANCHOR: auth_url
    challenge = b64url(hashlib.sha256(VERIFIER.encode()).digest())
    auth_params = {
        "response_type": "code", "client_id": CLIENT_ID, "redirect_uri": REDIRECT,
        "scope": SCOPE, "state": STATE, "code_challenge": challenge,
        "code_challenge_method": "S256", "resource": RESOURCE,
    }
    print("[5] 授权 URL:", AS_METADATA["authorization_endpoint"])
    for key, value in auth_params.items():
        print(f"    {key} = {value}")
    # ANCHOR_END: auth_url

    callback = {"code": CODE, "iss": AS, "state": STATE}
    print("[6] 回调（RFC 9207）:", urlencode(callback))
    callback_checks = [
        ("iss", callback["iss"] == AS_METADATA["issuer"]),
        ("state", callback["state"] == STATE),
    ]
    print("[7] 校验回调")
    for name, ok in callback_checks:
        print(f"  {name:<9} {'OK' if ok else '失败'}")
    if not all(ok for _, ok in callback_checks):
        print("回调校验失败，不换 token")
        return

    token_request = {
        "grant_type": "authorization_code", "code": CODE, "redirect_uri": REDIRECT,
        "client_id": CLIENT_ID, "code_verifier": VERIFIER, "resource": RESOURCE,
    }
    print("[8] token 请求:", json.dumps(token_request, ensure_ascii=False, indent=1))
    access = {"access_token": "at-demo", "token_type": "Bearer", "expires_in": 600,
              "scope": SCOPE, "aud": RESOURCE, "iss": AS}
    print("[9] token 响应: aud =", access["aud"])

    # ANCHOR: checks
    checks = [
        ("PKCE", b64url(hashlib.sha256(token_request["code_verifier"].encode()).digest())
         == auth_params["code_challenge"]),
        ("resource", token_request["resource"] == PRM["resource"] == access["aud"]),
        ("aud", access["aud"] == RESOURCE),
        ("scope", set(access["scope"].split()) >= set(SCOPE.split())),
    ]
    print("[10] 校验 token")
    for name, ok in checks:
        print(f"  {name:<9} {'OK' if ok else '失败'}")
    print("全部通过:", all(ok for _, ok in callback_checks + checks))
    # ANCHOR_END: checks


if __name__ == "__main__":
    main()
