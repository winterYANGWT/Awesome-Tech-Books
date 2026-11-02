/* 1.1 "从实验室到产品" 的交互层。
 *
 * 纯物理模型 hopperStep() 不碰 DOM，可以在 Node 里直接测试；其余部分只在
 * 页面存在 .qh-story 时运行。没有 JS 时，Markdown 正文和 <details> 仍然完整，
 * 图 1-1、图 1-2 由 .qh-fallback 显示。
 */
(function () {
    "use strict";

    var REDUCED = typeof window !== "undefined" && typeof window.matchMedia === "function" &&
        window.matchMedia("(prefers-reduced-motion: reduce)").matches;

    /* ------------------------------------------------------------------ *
     * 平面弹簧-质量跳跃机（纯函数）
     * ------------------------------------------------------------------ */

    var HOPPER_PARAMS = {
        m: 10,          // 机身质量 kg
        I: 0.3,         // 俯仰转动惯量 kg·m²
        L0: 0.5,        // 腿自由长度 m
        k: 3000,        // 腿刚度 N/m
        g: 9.81,        // 重力加速度 m/s²
        c: 8,           // 腿的阻尼 N·s/m，给高度控制一点耗散
        kThrust: 1350,  // 伸展段推力增益 N/m
        cap: 400,       // 推力上限 N
        kv: 0.05,       // 落脚点速度反馈 s
        kp: 600,        // 髋关节姿态比例增益 N·m/rad
        kd: 60,         // 髋关节姿态微分增益 N·m·s/rad
        travel: 0.9,    // 支撑期实际水平行程 / (vx·Ts) 的修正
        deadband: 0.004,
        hDes: 0.2,
        vDes: 0.4
    };

    function clamp(v, lo, hi) {
        return v < lo ? lo : (v > hi ? hi : v);
    }

    function createHopperState(params) {
        var p = params || HOPPER_PARAMS;
        var z = p.L0 + p.hDes;
        return {
            x: 0, z: z, vx: 0, vz: 0,
            theta: 0, omega: 0,
            phase: "flight",
            footX: 0, L: p.L0, Ldot: 0,
            Ts: 0.2, stanceT: 0,
            hApex: p.hDes, lastPeak: z,
            time: 0
        };
    }

    /* 一步半隐式欧拉。state 原地更新并返回，dt 单位为秒。 */
    function hopperStep(state, params, dt) {
        var p = params;
        var s = state;
        var g = p.g;

        if (s.phase === "flight") {
            s.vz -= g * dt;
            s.x += s.vx * dt;
            s.z += s.vz * dt;
            s.theta += s.omega * dt;
            s.time += dt;
            if (s.z > s.lastPeak) s.lastPeak = s.z;

            /* 落脚点：上一段支撑的水平行程的一半，加一个速度误差项。 */
            var dxf = p.travel * s.vx * s.Ts / 2 + p.kv * (s.vx - p.vDes);
            var ang = Math.asin(clamp(dxf / p.L0, -0.75, 0.75));
            if (s.z <= p.L0 * Math.cos(ang) && s.vz < 0) {
                s.phase = "stance";
                s.stanceT = 0;
                s.footX = s.x + dxf;
                var ddx = s.x - s.footX;
                s.L = Math.hypot(ddx, s.z);
                s.Ldot = s.vz * (s.z / s.L) + s.vx * (ddx / s.L);
                s.hApex = s.lastPeak - p.L0;
            }
        } else {
            var dx = s.x - s.footX;
            var Lnew = Math.hypot(dx, s.z);
            s.Ldot = (Lnew - s.L) / dt;
            s.L = Lnew;

            var F = 0;
            if (s.L < p.L0) F = p.k * (p.L0 - s.L) - p.c * s.Ldot;
            if (F < 0) F = 0;
            if (s.L < p.L0 && s.Ldot > 0) {
                var err = p.hDes - s.hApex;
                if (Math.abs(err) > p.deadband) {
                    F += clamp(p.kThrust * err, 0, p.cap);
                }
            }
            var ux = dx / s.L;
            var uz = s.z / s.L;
            s.vx += (F * ux / p.m) * dt;
            s.vz += (F * uz / p.m - g) * dt;
            s.x += s.vx * dt;
            s.z += s.vz * dt;

            var tau = -p.kp * s.theta - p.kd * s.omega;
            s.omega += tau / p.I * dt;
            s.theta += s.omega * dt;
            s.stanceT += dt;
            s.time += dt;

            if (Math.hypot(s.x - s.footX, s.z) >= p.L0 && s.stanceT > 0.01) {
                s.phase = "flight";
                s.Ts = s.stanceT;
                s.lastPeak = s.z;
                s.L = p.L0;
            }
        }
        return s;
    }

    if (typeof module !== "undefined" && module.exports) {
        module.exports = {
            HOPPER_PARAMS: HOPPER_PARAMS,
            createHopperState: createHopperState,
            hopperStep: hopperStep
        };
    }
    if (typeof document === "undefined") return;

    /* ------------------------------------------------------------------ *
     * 小工具
     * ------------------------------------------------------------------ */

    function esc(text) {
        return String(text).replace(/[&<>"]/g, function (ch) {
            return { "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[ch];
        });
    }

    function el(tag, cls, text) {
        var node = document.createElement(tag);
        if (cls) node.className = cls;
        if (text !== undefined) node.textContent = text;
        return node;
    }

    function sceneOf(node) {
        return node.closest ? node.closest(".qh-scene") : null;
    }

    /* 侧视轮廓，朝右。120×90 的坐标里画，脚在 y≈84。 */
    function robotSvg(form) {
        var body = "";
        if (form === "biped") {
            body =
                '<g class="qh-leg qh-far">' +
                '<path d="M54 24 L46 36 L48 46"/><path d="M60 44 L66 54 L58 64"/>' +
                '<circle class="qh-r-joint" cx="66" cy="54" r="2.2"/>' +
                "</g>" +
                '<rect class="qh-r-torso" x="54" y="18" width="16" height="27" rx="4"/>' +
                '<rect class="qh-r-head" x="57" y="6" width="11" height="11" rx="3"/>' +
                '<g class="qh-leg qh-near">' +
                '<path d="M70 24 L78 36 L76 46"/><path d="M68 44 L74 54 L66 64"/>' +
                '<circle class="qh-r-joint" cx="74" cy="54" r="2.2"/>' +
                "</g>" +
                '<path class="qh-r-foot" d="M54 64 h8 M62 64 h8"/>';
        } else if (form === "wheeled") {
            body =
                '<g class="qh-leg qh-far qh-far-rear">' +
                '<path d="M45 32 L36 48 L46 59"/><circle class="qh-r-joint" cx="36" cy="48" r="2.2"/>' +
                '<circle class="qh-r-wheel" cx="46" cy="59" r="7"/><circle class="qh-r-hub" cx="46" cy="59" r="1.6"/>' +
                "</g>" +
                '<g class="qh-leg qh-far qh-far-front">' +
                '<path d="M91 32 L82 48 L92 59"/><circle class="qh-r-joint" cx="82" cy="48" r="2.2"/>' +
                '<circle class="qh-r-wheel" cx="92" cy="59" r="7"/><circle class="qh-r-hub" cx="92" cy="59" r="1.6"/>' +
                "</g>" +
                '<rect class="qh-r-body" x="30" y="18" width="66" height="16" rx="5"/>' +
                '<rect class="qh-r-head" x="96" y="12" width="16" height="14" rx="4"/>' +
                '<g class="qh-leg qh-near qh-near-rear">' +
                '<path d="M40 32 L31 48 L41 59"/><circle class="qh-r-joint" cx="31" cy="48" r="2.2"/>' +
                '<circle class="qh-r-wheel" cx="41" cy="59" r="7"/><circle class="qh-r-hub" cx="41" cy="59" r="1.6"/>' +
                "</g>" +
                '<g class="qh-leg qh-near qh-near-front">' +
                '<path d="M86 32 L77 48 L87 59"/><circle class="qh-r-joint" cx="77" cy="48" r="2.2"/>' +
                '<circle class="qh-r-wheel" cx="87" cy="59" r="7"/><circle class="qh-r-hub" cx="87" cy="59" r="1.6"/>' +
                "</g>";
        } else {
            body =
                '<g class="qh-leg qh-far qh-far-rear">' +
                '<path d="M45 32 L36 48 L46 64"/><circle class="qh-r-joint" cx="36" cy="48" r="2.2"/>' +
                '<path class="qh-r-foot" d="M42 64 h8"/>' +
                "</g>" +
                '<g class="qh-leg qh-far qh-far-front">' +
                '<path d="M91 32 L82 48 L92 64"/><circle class="qh-r-joint" cx="82" cy="48" r="2.2"/>' +
                '<path class="qh-r-foot" d="M88 64 h8"/>' +
                "</g>" +
                '<rect class="qh-r-body" x="30" y="18" width="66" height="16" rx="5"/>' +
                '<rect class="qh-r-head" x="96" y="12" width="16" height="14" rx="4"/>' +
                '<g class="qh-leg qh-near qh-near-rear">' +
                '<path d="M40 32 L31 48 L41 64"/><circle class="qh-r-joint" cx="31" cy="48" r="2.2"/>' +
                '<path class="qh-r-foot" d="M37 64 h8"/>' +
                "</g>" +
                '<g class="qh-leg qh-near qh-near-front">' +
                '<path d="M86 32 L77 48 L87 64"/><circle class="qh-r-joint" cx="77" cy="48" r="2.2"/>' +
                '<path class="qh-r-foot" d="M83 64 h8"/>' +
                "</g>";
        }
        return '<svg class="qh-robot" viewBox="0 0 120 72" preserveAspectRatio="xMidYMax meet" aria-hidden="true" focusable="false">' +
            body + "</svg>";
    }

    function machineVisual(machine, images) {
        if ((images || []).indexOf(machine.slug + ".png") >= 0) {
            return '<img class="qh-robot-img" src="story/robots/' + encodeURIComponent(machine.slug) +
                '.png" alt="' + esc(machine.name) + ' 的轮廓" loading="lazy">';
        }
        return robotSvg(machine.form);
    }

    var DRIVE_LABEL = { hydraulic: "液压", electric: "电机" };
    var FORM_LABEL = { biped: "双足", quadruped: "四足", wheeled: "轮足" };

    /* ------------------------------------------------------------------ *
     * 顶部时间尺
     * ------------------------------------------------------------------ */

    function initRail(story) {
        var rail = story.querySelector(".qh-rail");
        if (!rail) return;
        var scenes = Array.prototype.slice.call(story.querySelectorAll(".qh-scene"));
        if (!scenes.length) return;
        var years = scenes.map(function (s) { return Number(s.dataset.year); });
        var MIN = 1960, MAX = 2024;
        var pct = function (y) { return ((y - MIN) / (MAX - MIN)) * 100; };
        var current = -1;
        var walkTimer = 0;

        var html = '<div class="qh-rail-inner"><div class="qh-ruler"><div class="qh-axis"></div>';
        scenes.forEach(function (s, i) {
            html += '<button type="button" class="qh-dot" data-scene="' + i +
                '" style="left:' + pct(years[i]).toFixed(3) + '%" aria-label="跳到 ' +
                esc(s.dataset.label) + "：" + esc(s.querySelector(".qh-title").textContent) + '"></button>';
        });
        [1960, 1980, 2000, 2024].forEach(function (y) {
            html += '<span class="qh-tick" style="left:' + pct(y).toFixed(3) + '%">' + y + "</span>";
        });
        html += '<div class="qh-marker" style="left:' + pct(years[0]).toFixed(3) + '%">' +
            robotSvg("quadruped") + "</div></div>" +
            '<div class="qh-now"><span class="qh-now-era"></span><span class="qh-now-title"></span></div></div>';
        rail.innerHTML = html;

        var marker = rail.querySelector(".qh-marker");
        var era = rail.querySelector(".qh-now-era");
        var title = rail.querySelector(".qh-now-title");
        var dots = Array.prototype.slice.call(rail.querySelectorAll(".qh-dot"));

        function setCurrent(i, animate) {
            if (i < 0 || i >= scenes.length || i === current) return;
            var moving = current >= 0 && animate !== false && !REDUCED;
            current = i;
            marker.style.left = pct(years[i]).toFixed(3) + "%";
            era.textContent = scenes[i].dataset.label;
            title.textContent = scenes[i].querySelector(".qh-title").textContent;
            dots.forEach(function (d, k) {
                d.classList.toggle("qh-on", k === i);
                if (k === i) d.setAttribute("aria-current", "true");
                else d.removeAttribute("aria-current");
            });
            if (moving) {
                marker.classList.add("qh-walking");
                clearTimeout(walkTimer);
                walkTimer = setTimeout(function () { marker.classList.remove("qh-walking"); }, 700);
            }
            if (typeof updateMargins === "function") updateMargins();
        }

        dots.forEach(function (d) {
            d.addEventListener("click", function () {
                var target = scenes[Number(d.dataset.scene)];
                target.scrollIntoView({ behavior: REDUCED ? "auto" : "smooth", block: "start" });
                setCurrent(Number(d.dataset.scene), true);
            });
        });

        function updateMargins() {
            var bar = parseFloat(getComputedStyle(document.documentElement)
                .getPropertyValue("--menu-bar-height")) || 50;
            var margin = bar + rail.offsetHeight;
            scenes.forEach(function (s) { s.style.scrollMarginTop = margin + "px"; });
        }

        setCurrent(0, false);
        updateMargins();
        window.addEventListener("resize", updateMargins);
        window.addEventListener("load", updateMargins);

        if (typeof IntersectionObserver !== "function") {
            return;
        }
        var observer = new IntersectionObserver(function (entries) {
            entries.forEach(function (entry) {
                if (entry.isIntersecting) {
                    setCurrent(scenes.indexOf(entry.target), true);
                }
            });
        }, { rootMargin: "-42% 0px -53% 0px", threshold: 0 });
        scenes.forEach(function (s) { observer.observe(s); });
    }

    /* ------------------------------------------------------------------ *
     * 入场
     * ------------------------------------------------------------------ */

    function initEntrance(story) {
        if (REDUCED || typeof IntersectionObserver !== "function") return;
        var scenes = Array.prototype.slice.call(story.querySelectorAll(".qh-scene"));
        var observer;
        try {
            observer = new IntersectionObserver(function (entries) {
                entries.forEach(function (entry) {
                    if (entry.isIntersecting) {
                        entry.target.classList.add("qh-in");
                        observer.unobserve(entry.target);
                    }
                });
            }, { rootMargin: "0px 0px -8% 0px", threshold: 0.02 });
        } catch (err) {
            return;
        }
        scenes.forEach(function (s) {
            s.classList.add("qh-anim");
            observer.observe(s);
        });

        /* 兜底：observer 已经建好，但如果回调没有按时来（极端浏览器），
           滚动时按位置自己把内容显示出来，绝不留下看不见的正文。 */
        var queued = false;
        function revealVisible() {
            scenes.forEach(function (s) {
                if (s.classList.contains("qh-in")) return;
                var box = s.getBoundingClientRect();
                if (box.top < window.innerHeight * 1.05 && box.bottom > -200) {
                    s.classList.add("qh-in");
                }
            });
        }
        function onScroll() {
            if (queued) return;
            queued = true;
            requestAnimationFrame(function () { queued = false; revealVisible(); });
        }
        window.addEventListener("scroll", onScroll, { passive: true });
        window.addEventListener("resize", onScroll);
        revealVisible();
        setTimeout(revealVisible, 3000);
    }

    /* ------------------------------------------------------------------ *
     * 第一幕：三个节点
     * ------------------------------------------------------------------ */

    function initMilestones(root) {
        root.innerHTML =
            '<ol class="qh-miles">' +
            '<li><span class="qh-mile-year">1960s</span><span class="qh-mile-name">液压步行卡车</span>' +
            '<span class="qh-mile-note">四条腿由液压驱动</span></li>' +
            '<li><span class="qh-mile-year">1970s</span><span class="qh-mile-name">双足步行机器人</span>' +
            '<span class="qh-mile-note">日本做出双足步行机器人</span></li>' +
            '<li><span class="qh-mile-year">1980s</span><span class="qh-mile-name">Raibert 把“跑”拆成三个问题</span>' +
            '<span class="qh-mile-note">弹跳高度、前进速度、机身姿态</span></li>' +
            "</ol>";
        var items = Array.prototype.slice.call(root.querySelectorAll(".qh-miles li"));
        if (REDUCED || typeof IntersectionObserver !== "function") {
            items.forEach(function (li) { li.classList.add("qh-on"); });
            return;
        }
        var observer = new IntersectionObserver(function (entries) {
            if (!entries.some(function (e) { return e.isIntersecting; })) return;
            observer.disconnect();
            items.forEach(function (li, i) {
                setTimeout(function () { li.classList.add("qh-on"); }, i * 700);
            });
        }, { threshold: 0.35 });
        observer.observe(root);
    }

    /* ------------------------------------------------------------------ *
     * 弹跳演示
     * ------------------------------------------------------------------ */

    function initHopper(root) {
        var params = Object.assign({}, HOPPER_PARAMS);
        var state = createHopperState(params);
        var running = !REDUCED;
        var visible = true;
        var frame = 0;
        var last = 0;
        var trail = [];

        root.innerHTML =
            '<div class="qh-hop-stage">' +
            '<svg class="qh-hop-svg" viewBox="-2 0 4.1 2" preserveAspectRatio="xMidYMid meet" role="img" aria-label="弹簧-质量跳跃机侧视仿真">' +
            '<defs><pattern id="qh-ground-pattern" width="1" height="0.1" patternUnits="userSpaceOnUse">' +
            '<line class="qh-major" x1="0" y1="0" x2="0" y2="0.08"/>' +
            '<line x1="0.25" y1="0" x2="0.25" y2="0.04"/>' +
            '<line x1="0.5" y1="0" x2="0.5" y2="0.04"/>' +
            '<line x1="0.75" y1="0" x2="0.75" y2="0.04"/>' +
            '</pattern></defs>' +
            '<g class="qh-hop-ground"><line x1="-1000" y1="1.9" x2="1000" y2="1.9"/>' +
            '<rect x="-1000" y="1.9" width="2000" height="0.1" fill="url(#qh-ground-pattern)"/></g>' +
            '<line class="qh-hop-target" x1="-1000" x2="1000" y1="1.2" y2="1.2"/>' +
            '<polyline class="qh-hop-tail" points=""/>' +
            '<g class="qh-hop-body"><rect x="-0.3" y="-0.12" width="0.6" height="0.24" rx="0.05"/></g>' +
            '<path class="qh-hop-leg" d=""/>' +
            '<circle class="qh-hop-foot" cx="0" cy="1.9" r="0.03"/>' +
            "</svg>" +
            '<div class="qh-hop-phase">腾空</div>' +
            "</div>" +
            '<p class="qh-hop-warn">示意模型，参数不是 A1 的。</p>' +
            '<div class="qh-hop-controls">' +
            '<label class="qh-hop-slider"><span>弹跳高度 <b class="qh-h-out">0.20</b> m</span>' +
            '<input type="range" min="0.05" max="0.30" step="0.01" value="0.20" data-hop="h"></label>' +
            '<label class="qh-hop-slider"><span>前进速度 <b class="qh-v-out">0.40</b> m/s</span>' +
            '<input type="range" min="-0.5" max="1.5" step="0.1" value="0.4" data-hop="v"></label>' +
            '<div class="qh-hop-buttons"><button type="button" data-hop="push">推一下</button>' +
            '<button type="button" data-hop="pause">' + (running ? "暂停" : "继续") + "</button></div>" +
            "</div>" +
            '<div class="qh-hop-readout">' +
            '<div><span>弹跳高度</span><b class="qh-r-h">0.20</b><i>目标 <em class="qh-r-ht">0.20</em> m</i></div>' +
            '<div><span>前进速度</span><b class="qh-r-v">0.00</b><i>目标 <em class="qh-r-vt">0.40</em> m/s</i></div>' +
            '<div><span>机身姿态</span><b class="qh-r-t">0.00</b><i>目标 0.00 rad</i></div>' +
            "</div>" +
            '<p class="qh-hop-note">高度靠腿的伸缩和触地时机，速度靠落脚点相对髋部的偏移，姿态靠髋力矩。</p>';

        var svg = root.querySelector(".qh-hop-svg");
        var body = root.querySelector(".qh-hop-body");
        var leg = root.querySelector(".qh-hop-leg");
        var foot = root.querySelector(".qh-hop-foot");
        var target = root.querySelector(".qh-hop-target");
        var tailLine = root.querySelector(".qh-hop-tail");
        var phase = root.querySelector(".qh-hop-phase");
        var rH = root.querySelector(".qh-r-h");
        var rV = root.querySelector(".qh-r-v");
        var rT = root.querySelector(".qh-r-t");
        var ht = root.querySelector(".qh-r-ht");
        var vt = root.querySelector(".qh-r-vt");
        var hOut = root.querySelector(".qh-h-out");
        var vOut = root.querySelector(".qh-v-out");
        var pauseBtn = root.querySelector('[data-hop="pause"]');

        function springPath(x1, y1, x2, y2) {
            var dx = x2 - x1, dy = y2 - y1;
            var len = Math.hypot(dx, dy) || 1;
            var ux = dx / len, uy = dy / len;
            var nx = -uy, ny = ux;
            var coils = 6, amp = 0.035, d = "M" + x1.toFixed(3) + " " + y1.toFixed(3);
            for (var i = 1; i <= coils; i++) {
                var f = i / (coils + 1);
                var off = (i % 2 ? 1 : -1) * amp;
                d += " L" + (x1 + ux * len * f + nx * off).toFixed(3) + " " +
                    (y1 + uy * len * f + ny * off).toFixed(3);
            }
            return d + " L" + x2.toFixed(3) + " " + y2.toFixed(3);
        }

        function render() {
            var box = svg.getBoundingClientRect();
            var vw = (typeof window !== "undefined" && window.innerWidth <= 680) ? 2.0 : 3.0;
            var vh = 1.5;
            if (box.width > 0 && box.height > 0) vh = vw * box.height / box.width;
            if (!isFinite(vh) || vh < 0.8) vh = 1.5;
            var y0 = 2.02 - vh;
            var camX = state.x - vw / 2;
            svg.setAttribute("viewBox", camX.toFixed(3) + " " + y0.toFixed(3) + " " + vw.toFixed(2) + " " + vh.toFixed(3));

            var bx = state.x;
            var by = 1.9 - state.z;
            body.setAttribute("transform", "translate(" + bx.toFixed(3) + " " + by.toFixed(3) + ") rotate(" +
                (-state.theta * 180 / Math.PI).toFixed(2) + ")");

            var fx, fy;
            if (state.phase === "stance") {
                fx = state.footX;
                fy = 1.9;
            } else {
                var dxf = params.travel * state.vx * state.Ts / 2 + params.kv * (state.vx - params.vDes);
                var ang = Math.asin(clamp(dxf / params.L0, -0.75, 0.75));
                fx = bx + Math.sin(ang) * state.L;
                fy = by + Math.cos(ang) * state.L;
            }
            leg.setAttribute("d", springPath(bx, by + 0.1, fx, fy));
            foot.setAttribute("cx", fx.toFixed(3));
            foot.setAttribute("cy", fy.toFixed(3));

            var ty = (1.9 - (params.L0 + params.hDes)).toFixed(3);
            target.setAttribute("y1", ty);
            target.setAttribute("y2", ty);

            var points = [];
            for (var i = 0; i < trail.length; i++) {
                points.push(trail[i].x.toFixed(3) + "," + (1.9 - trail[i].z).toFixed(3));
            }
            tailLine.setAttribute("points", points.join(" "));
        }

        function readout() {
            rH.textContent = state.hApex.toFixed(2);
            rV.textContent = state.vx.toFixed(2);
            rT.textContent = state.theta.toFixed(2);
            ht.textContent = params.hDes.toFixed(2);
            vt.textContent = params.vDes.toFixed(1);
            if (state.phase === "flight") phase.textContent = "腾空";
            else phase.textContent = state.Ldot < 0 ? "压缩" : "伸展";
            phase.dataset.phase = state.phase === "flight" ? "flight" : (state.Ldot < 0 ? "compress" : "extend");
        }

        function tick(now) {
            frame = 0;
            if (!running || !visible) return;
            if (!last) last = now;
            var dt = Math.min(0.05, (now - last) / 1000);
            last = now;
            var steps = Math.max(1, Math.round(dt * 1000));
            for (var i = 0; i < steps; i++) hopperStep(state, params, 0.001);
            if (!Number.isFinite(state.z) || state.z < 0 || state.z > 12) {
                state = createHopperState(params);
                trail = [];
            }
            trail.push({ t: state.time, x: state.x, z: state.z });
            while (trail.length > 2 && state.time - trail[0].t > 2) trail.shift();
            render();
            readout();
            frame = requestAnimationFrame(tick);
        }

        function start() {
            if (frame) return;
            last = 0;
            frame = requestAnimationFrame(tick);
        }

        function stop() {
            if (frame) cancelAnimationFrame(frame);
            frame = 0;
            last = 0;
        }

        root.querySelector('[data-hop="h"]').addEventListener("input", function (e) {
            params.hDes = Number(e.target.value);
            hOut.textContent = params.hDes.toFixed(2);
            render();
        });
        root.querySelector('[data-hop="v"]').addEventListener("input", function (e) {
            params.vDes = Number(e.target.value);
            vOut.textContent = params.vDes.toFixed(1);
            render();
        });
        root.querySelector('[data-hop="push"]').addEventListener("click", function () {
            state.omega += 1.0;
            if (!running) { running = true; pauseBtn.textContent = "暂停"; start(); }
        });
        pauseBtn.addEventListener("click", function () {
            running = !running;
            pauseBtn.textContent = running ? "暂停" : "继续";
            if (running) start(); else stop();
        });

        if (typeof IntersectionObserver === "function") {
            new IntersectionObserver(function (entries) {
                visible = entries[0].isIntersecting;
                if (visible && running) start(); else stop();
            }, { threshold: 0.05 }).observe(root);
        }
        document.addEventListener("visibilitychange", function () {
            if (document.hidden) stop();
            else if (running && visible) start();
        });

        trail.push({ t: state.time, x: state.x, z: state.z });
        render();
        readout();
        window.addEventListener("resize", render);
        requestAnimationFrame(render);
        if (running) start();
    }

    /* ------------------------------------------------------------------ *
     * 第三幕：四张零件卡片
     * ------------------------------------------------------------------ */

    function initParts(root) {
        var cards = [
            { name: "液压", q: "能提供大力矩，但效率低、管路重", l: "后来", w: "液压伺服、阀控缸和模型控制让机器能背负重物、在泥地里行走" },
            { name: "电机", q: "力矩密度不够", w: "永磁材料、功率器件和磁场定向控制（FOC）成熟" },
            { name: "计算", q: "算不动整机动力学", w: "嵌入式计算平台能实时算完整机动力学" },
            { name: "电池", q: "背不动、跑不久", w: "能量密度上来" }
        ];
        var stages = [
            { year: "1990s", text: "四足机器大多只能在实验室里走几步" },
            { year: "2005 前后", text: "BigDog 这类液压平台证明了野外行走的可能" },
            { year: "2010 以后", text: "电机、驱动和计算同时降价，四足真正进入产品化阶段" }
        ];
        var html = '<div class="qh-parts">';
        cards.forEach(function (c) {
            html += '<div class="qh-part"><h5>' + c.name + '</h5><p><span>问题</span>' + c.q +
                '</p><p><span>' + (c.l || "等到") + '</span>' + c.w + "</p></div>";
        });
        html += "</div>" + '<div class="qh-stages" role="group" aria-label="零件成熟的时间">';
        stages.forEach(function (s, i) {
            html += '<button type="button" data-stage="' + i + '"' + (i === 0 ? ' class="qh-on"' : "") +
                ">" + s.year + "</button>";
        });
        html += '</div><p class="qh-stage-text">' + stages[0].text + "</p>";
        root.innerHTML = html;

        var buttons = Array.prototype.slice.call(root.querySelectorAll("[data-stage]"));
        var text = root.querySelector(".qh-stage-text");
        buttons.forEach(function (b) {
            b.addEventListener("click", function () {
                buttons.forEach(function (o) { o.classList.remove("qh-on"); });
                b.classList.add("qh-on");
                text.textContent = stages[Number(b.dataset.stage)].text;
            });
        });
    }

    /* ------------------------------------------------------------------ *
     * 第四幕：三种驱动与形态
     * ------------------------------------------------------------------ */

    function initForms(root) {
        var columns = [
            {
                title: "液压四足", names: "HyQ", form: "quadruped", tone: "hydraulic",
                plus: "功率密度、抗冲击", minus: "泄漏、维护和效率"
            },
            {
                title: "电机四足", names: "Spot / A1 / ANYmal", form: "quadruped", tone: "electric",
                plus: "控制精度、成本和批量一致性", minus: "峰值力矩受减速器和散热限制"
            },
            {
                title: "轮足", names: "B2-W", form: "wheeled", tone: "electric",
                plus: "平地的高效率交给轮子，越障和调姿交给腿", minus: "结构复杂、控制模式多"
            }
        ];
        var html = '<div class="qh-forms">';
        columns.forEach(function (c) {
            html += '<div class="qh-form qh-tone-' + c.tone + '">' +
                '<div class="qh-form-art">' + robotSvg(c.form) + "</div>" +
                '<h5>' + c.title + '</h5><p class="qh-form-names">' + c.names + "</p>" +
                '<p class="qh-form-plus"><span>优势</span>' + c.plus + "</p>" +
                '<p class="qh-form-minus"><span>代价</span>' + c.minus + "</p></div>";
        });
        html += "</div>" +
            '<p class="qh-forms-note">三种组合同时比较：驱动方式决定了力矩密度、带宽和维护方式，形态决定了它适合的地形和任务。</p>';
        root.innerHTML = html;
    }

    /* ------------------------------------------------------------------ *
     * 第五幕：十二个节点
     * ------------------------------------------------------------------ */

    function initTimeline(root, story) {
        var elData = document.getElementById("qh-timeline-data");
        var data = { machines: [], images: [] };
        if (elData) {
            try { data = JSON.parse(elData.textContent); } catch (err) { data = { machines: [], images: [] }; }
        }
        var machines = (data.machines || []).slice().sort(function (a, b) { return a.year - b.year; });
        if (!machines.length) {
            root.innerHTML = '<p class="qh-empty">时间轴数据没有加载。</p>';
            return false;
        }
        var selected = 0;
        var filter = { kind: "all", value: null };
        var playing = false;
        var playTimer = 0;
        var playIndex = 0;

        var TEAM_INTRO = {
            "Boston Dynamics": "由 Raibert 创办；产品线覆盖双足 Atlas、四足 Spot 和轮式底盘加机械臂的 Stretch。",
            "MIT": "负责人 Sangbae Kim，实验室名为 Biomimetic Robotics。",
            "ETH": "Robotic Systems 团队，代表平台 ANYmal，在模型控制和工业巡检上积累很深。",
            "IIT": "走液压路线，团队名为 Dynamic Legged Systems，代表平台是 HyQ 系列。",
            "Unitree": "宇树科技，从 2017 年公开零售 Laikago 开始，用高功率密度关节电机和低成本量产打开市场。"
        };

        root.innerHTML =
            '<div class="qh-tl-filters" role="group" aria-label="筛选机型">' +
            '<button type="button" class="qh-on" data-filter="all" aria-pressed="true">全部</button>' +
            '<button type="button" data-filter="drive" data-value="hydraulic" aria-pressed="false">液压</button>' +
            '<button type="button" data-filter="drive" data-value="electric" aria-pressed="false">电机</button>' +
            '<button type="button" data-filter="open" aria-pressed="false">开源</button>' +
            '<span class="qh-tl-divider" aria-hidden="true"></span>' +
            ["Boston Dynamics", "IIT", "ETH", "MIT", "Unitree"].map(function (t) {
                return '<button type="button" data-filter="team" data-value="' + esc(t) + '" aria-pressed="false">' + esc(t) + "</button>";
            }).join("") +
            "</div>" +
            '<p class="qh-tl-note">第六支团队是宾夕法尼亚大学 GRASP，和 Ghost Robotics 合作把多款四足平台推向商业化；这十二个样本里没有收录它们的机型。</p>' +
            '<div class="qh-tl-tools"><button type="button" class="qh-tl-play" data-play aria-pressed="false">▶ 播放时间线</button></div>' +
            '<div class="qh-tl-team" hidden></div>' +
            '<div class="qh-tl-scroll"><div class="qh-tl-track"></div></div>' +
            '<div class="qh-tl-detail" aria-live="polite"></div>' +
            '<div class="qh-stats">' +
            '<div><span>样本机型数</span><b data-stat="n">0</b><i>台</i></div>' +
            '<div><span>电机驱动占比</span><b data-stat="e">0</b><i>0/0</i></div>' +
            '<div><span>液压驱动占比</span><b data-stat="h">0</b><i>0/0</i></div>' +
            '<div><span>开源机型数</span><b data-stat="o">0</b><i>台</i></div>' +
            "</div>" +
            '<p class="qh-stats-mark" hidden>当前筛选</p>';

        var track = root.querySelector(".qh-tl-track");
        var scrollEl = root.querySelector(".qh-tl-scroll");
        var detail = root.querySelector(".qh-tl-detail");
        var mark = root.querySelector(".qh-stats-mark");
        var teamBox = root.querySelector(".qh-tl-team");
        var playBtn = root.querySelector("[data-play]");
        var buttons = [];

        machines.forEach(function (m, i) {
            var b = el("button", "qh-tl-node");
            b.type = "button";
            b.dataset.index = String(i);
            b.className += " qh-drive-" + m.drive + (m.open ? " qh-open" : "");
            b.setAttribute("aria-pressed", "false");
            b.setAttribute("title", m.year + " · " + m.team + " · " + m.name);
            b.innerHTML = '<span class="qh-tl-icon">' + machineVisual(m, data.images) + "</span>" +
                '<span class="qh-tl-year">' + m.year + "</span>" +
                '<span class="qh-tl-name">' + esc(m.name) + "</span>" +
                (m.open ? '<span class="qh-tl-star" aria-label="开源">★</span>' : "") +
                '<span class="qh-tl-dot" aria-hidden="true"></span>';
            b.addEventListener("click", function () {
                stopPlay();
                select(i);
            });
            track.appendChild(b);
            buttons.push(b);
        });

        function ensureVisible(btn) {
            if (!scrollEl || scrollEl.scrollWidth <= scrollEl.clientWidth) return;
            var left = btn.offsetLeft + btn.offsetWidth / 2 - scrollEl.clientWidth / 2;
            left = Math.max(0, Math.min(left, scrollEl.scrollWidth - scrollEl.clientWidth));
            if (scrollEl.scrollTo) scrollEl.scrollTo({ left: left, behavior: REDUCED ? "auto" : "smooth" });
            else scrollEl.scrollLeft = left;
        }

        function select(i) {
            if (i < 0 || i >= machines.length) return;
            selected = i;
            buttons.forEach(function (b, k) {
                b.classList.toggle("qh-selected", k === i);
                b.setAttribute("aria-pressed", k === i ? "true" : "false");
            });
            var m = machines[i];
            detail.innerHTML =
                '<div class="qh-tl-detail-head">' + m.year + " · " + esc(m.team) + " · " + esc(m.name) + "</div>" +
                '<div class="qh-tl-detail-body"><div class="qh-tl-detail-art qh-tone-' + (m.drive === "hydraulic" ? "hydraulic" : "electric") + '">' + machineVisual(m, data.images) + "</div>" +
                "<dl>" +
                "<dt>驱动</dt><dd>" + (DRIVE_LABEL[m.drive] || m.drive) + "</dd>" +
                "<dt>形态</dt><dd>" + (FORM_LABEL[m.form] || m.form) + "</dd>" +
                "<dt>开放情况</dt><dd>" + (m.open ? "开源" : "未公开") + "</dd>" +
                "<dt>备注</dt><dd>" + esc(m.note) + "</dd>" +
                "</dl></div>";
            ensureVisible(buttons[i]);
        }

        function matches(m) {
            if (filter.kind === "all") return true;
            if (filter.kind === "drive") return m.drive === filter.value;
            if (filter.kind === "open") return m.open;
            if (filter.kind === "team") return m.team === filter.value;
            return true;
        }

        function update() {
            var visible = [];
            machines.forEach(function (m, i) {
                var show = matches(m);
                buttons[i].hidden = !show;
                if (show) visible.push(i);
            });
            if (visible.length && visible.indexOf(selected) < 0) select(visible[0]);
            var subset = machines.filter(matches);
            var e = subset.filter(function (m) { return m.drive === "electric"; }).length;
            var h = subset.filter(function (m) { return m.drive === "hydraulic"; }).length;
            var o = subset.filter(function (m) { return m.open; }).length;
            var n = subset.length;
            root.querySelector('[data-stat="n"]').textContent = String(n);
            var eNode = root.querySelector('[data-stat="e"]');
            var hNode = root.querySelector('[data-stat="h"]');
            eNode.textContent = n ? e + "/" + n : "—";
            eNode.nextElementSibling.textContent = n ? "= " + (e / n * 100).toFixed(1) + "%" : "";
            hNode.textContent = n ? h + "/" + n : "—";
            hNode.nextElementSibling.textContent = n ? "= " + (h / n * 100).toFixed(1) + "%" : "";
            root.querySelector('[data-stat="o"]').textContent = String(o);
            mark.hidden = filter.kind === "all";
        }

        function applyFilter(kind, value) {
            filter = kind === "all" ? { kind: "all", value: null } : { kind: kind, value: value };
            root.querySelectorAll("[data-filter]").forEach(function (o) {
                var on = o.dataset.filter === kind && (kind === "all" || o.dataset.value === value);
                o.classList.toggle("qh-on", on);
                o.setAttribute("aria-pressed", on ? "true" : "false");
            });
            update();
            if (kind === "team") {
                teamBox.textContent = TEAM_INTRO[value] || "";
                teamBox.hidden = false;
            } else {
                teamBox.hidden = true;
                teamBox.textContent = "";
            }
        }

        function stopPlay() {
            if (playTimer) {
                clearInterval(playTimer);
                playTimer = 0;
            }
            playing = false;
            playBtn.textContent = "▶ 播放时间线";
            playBtn.setAttribute("aria-pressed", "false");
        }

        function startPlay() {
            stopPlay();
            applyFilter("all", null);
            playing = true;
            playIndex = 0;
            playBtn.textContent = "■ 停止播放";
            playBtn.setAttribute("aria-pressed", "true");
            select(0);
            playTimer = setInterval(function () {
                playIndex += 1;
                select(playIndex);
                if (playIndex >= machines.length - 1) stopPlay();
            }, 1400);
        }

        root.querySelectorAll("[data-filter]").forEach(function (b) {
            b.addEventListener("click", function () {
                stopPlay();
                if (b.dataset.filter === "all") applyFilter("all", null);
                else applyFilter(b.dataset.filter, b.dataset.value || null);
            });
        });

        playBtn.addEventListener("click", function () {
            if (playing) stopPlay(); else startPlay();
        });

        track.addEventListener("keydown", function (e) {
            if (e.key !== "ArrowRight" && e.key !== "ArrowLeft") return;
            var visible = buttons.filter(function (b) { return !b.hidden; });
            var at = visible.indexOf(document.activeElement);
            if (at < 0) return;
            var next = (at + (e.key === "ArrowRight" ? 1 : -1) + visible.length) % visible.length;
            visible[next].focus();
            e.preventDefault();
        });

        select(0);
        update();

        if (story && elData) elData.hidden = true;
        return true;
    }

    /* ------------------------------------------------------------------ *
     * 第六幕：开放程度的四层
     * ------------------------------------------------------------------ */

    function initOpenness(root) {
        var levels = [
            {
                name: "论文和视频", short: "能看不能跑",
                text: "只公开论文和视频，能看不能跑。读者可以照着复现思路，但拿不到能运行的代码。",
                who: []
            },
            {
                name: "SDK", short: "能调接口，看不到内部",
                text: "能调用接口但看不到控制内部。宇树提供 SDK 和部分例程，但没有公开整机硬件和底层固件；Spot 是商业整机加开发者接口，核心控制不公开。",
                who: ["宇树", "Spot"]
            },
            {
                name: "控制算法源码", short: "能改控制器，硬件是黑盒",
                text: "能改控制器，硬件仍是黑盒。Mini Cheetah 属于这一层：控制算法公开，2019 年底放出。",
                who: ["Mini Cheetah"]
            },
            {
                name: "硬件图纸 / 物料清单 / 固件", short: "能复制整机",
                text: "最深的一层：硬件图纸、物料清单和固件公开，能复制整机。Mini Cheetah 公开了硬件设计，把后两层往前推了一步。",
                who: ["Mini Cheetah"]
            }
        ];
        var html = '<div class="qh-dims"><span>驱动方式</span><span>开放程度</span><span>形态</span></div>' +
            '<ol class="qh-ladder">';
        levels.forEach(function (l, i) {
            html += '<li style="--i:' + i + '"><button type="button" data-level="' + i + '"' +
                (i === 1 ? ' class="qh-on"' : "") + '><b>' + l.name + "</b><span>" + l.short + "</span>" +
                (l.who.length ? '<em>' + l.who.join(" · ") + "</em>" : "") + "</button></li>";
        });
        html += "</ol>" +
            '<div class="qh-open-detail"><p class="qh-open-text">' + levels[1].text + "</p>" +
            '<p class="qh-open-who">Mini Cheetah 到第 4 层（硬件设计公开）；宇树到第 2 层（SDK，但整机硬件和底层固件不公开）；Spot 到第 2 层（商业接口，核心控制不公开）。</p></div>';
        root.innerHTML = html;

        var buttons = Array.prototype.slice.call(root.querySelectorAll("[data-level]"));
        var text = root.querySelector(".qh-open-text");
        buttons.forEach(function (b) {
            b.addEventListener("click", function () {
                buttons.forEach(function (o) { o.classList.remove("qh-on"); });
                b.classList.add("qh-on");
                text.textContent = levels[Number(b.dataset.level)].text;
            });
        });
    }

    /* ------------------------------------------------------------------ *
     * 第七幕：A1 与 Go1 教育版
     * ------------------------------------------------------------------ */

    function initA1Go1(root) {
        var a1 = [
            "关节电机额定 24 V",
            "模型给出的峰值力矩上限 33.5 N·m",
            "12 个关节，每条腿 3 个自由度",
            "更小、更便宜，适合单机控制和教学",
            "没有深度相机，视觉或导航会被传感器卡住",
            "本书的仿真对象"
        ];
        var go1 = [
            "机载计算机和传感器更完整",
            "能跑建图、定位和导航",
            "原书实机导航的载体",
            "非教育版没有二次开发权限，买错版本第 11 章无法进行",
            "与 A1 在运动学结构上一致，差别主要在传感器和机载计算机"
        ];
        function card(title, sub, items, tone) {
            return '<div class="qh-pair-card qh-tone-' + tone + '"><div class="qh-pair-art">' +
                robotSvg("quadruped") + "</div><h5>" + title + "</h5><p class=\"qh-pair-sub\">" + sub +
                "</p><ul>" + items.map(function (x) { return "<li>" + x + "</li>"; }).join("") + "</ul></div>";
        }
        root.innerHTML = '<div class="qh-pair">' +
            card("A1", "本书仿真用", a1, "electric") +
            card("Go1 教育版", "本书实机导航用", go1, "plain") +
            "</div>" +
            '<p class="qh-pair-note">把仿真控制器迁到 Go1 时，至少要把质量、腿长、关节零位和电机方向重新核对一遍。</p>';
    }

    /* ------------------------------------------------------------------ *
     * 第八幕：三条能力曲线
     * ------------------------------------------------------------------ */

    function initCurves(root) {
        var curves = [
            {
                key: "drive", title: "驱动", sub: "三个并列指标",
                metrics: ["力矩密度", "带宽", "效率"],
                link: "ch02.html", linkText: "第 2 章 关节电机",
                text: "驱动曲线看关节的力矩密度、带宽和效率，三个指标并列，没有先后或高低。"
            },
            {
                key: "control", title: "控制", sub: "三组并列转变",
                pairs: [["位置控制", "力矩控制"], ["准静态步态", "动态步态"], ["模型控制", "学习控制"]],
                link: "ch02.html", linkText: "第 2–10 章 控制",
                text: "控制曲线看三组转变：位置控制到力矩控制、准静态步态到动态步态、模型控制到学习控制。"
            },
            {
                key: "sense", title: "感知", sub: "一条转变",
                pairs: [["本体感受（编码器、IMU、足端接触）", "视觉、激光雷达、地图"]],
                link: "ch11.html", linkText: "第 11 章 感知与导航",
                text: "感知曲线看从本体感受（编码器、IMU、足端接触）到视觉、激光雷达和地图的变化。"
            }
        ];

        var html = '<div class="qh-curves">';
        curves.forEach(function (c) {
            var body = "";
            if (c.metrics) {
                body = '<span class="qh-curve-body qh-curve-metrics">' + c.metrics.map(function (m) {
                    return "<span>" + m + "</span>";
                }).join("") + "</span>";
            } else {
                body = '<span class="qh-curve-body">' + c.pairs.map(function (pair) {
                    return '<span class="qh-curve-pair"><span>' + pair[0] + "</span><em>→</em><span>" +
                        pair[1] + "</span></span>";
                }).join("") + "</span>";
            }
            html += '<button type="button" class="qh-curve" data-curve="' + c.key + '" aria-pressed="false">' +
                '<span class="qh-curve-head"><b>' + c.title + "</b><i>" + c.sub + "</i></span>" +
                body + "</button>";
        });
        html += '</div><div class="qh-curve-detail" aria-live="polite"></div>';
        root.innerHTML = html;

        var buttons = Array.prototype.slice.call(root.querySelectorAll(".qh-curve"));
        var detail = root.querySelector(".qh-curve-detail");
        function show(c) {
            detail.innerHTML = "<p>" + c.text + '</p><p><a href="' + c.link + '">' + c.linkText + "</a></p>";
        }
        buttons.forEach(function (b, i) {
            b.addEventListener("click", function () {
                var on = b.classList.contains("qh-on");
                buttons.forEach(function (o) {
                    o.classList.remove("qh-on");
                    o.setAttribute("aria-pressed", "false");
                });
                if (on) {
                    detail.innerHTML = "";
                    return;
                }
                b.classList.add("qh-on");
                b.setAttribute("aria-pressed", "true");
                show(curves[i]);
            });
        });
        buttons[1].classList.add("qh-on");
        buttons[1].setAttribute("aria-pressed", "true");
        show(curves[1]);
    }

    /* ------------------------------------------------------------------ *
     * 初始化
     * ------------------------------------------------------------------ */

    function init() {
        var story = document.querySelector(".qh-story");
        if (!story) return;
        /* 老浏览器没有 :has() 时，sticky 的祖先不能是 overflow:auto。 */
        var content = story.closest ? story.closest(".content") : null;
        if (content) content.style.overflowY = "visible";
        Array.prototype.slice.call(story.querySelectorAll(".qh-title")).forEach(function (t) {
            t.setAttribute("role", "heading");
            t.setAttribute("aria-level", "3");
        });
        var widgets = {
            milestones: initMilestones,
            hopper: initHopper,
            parts: initParts,
            forms: initForms,
            timeline: function (node) { return initTimeline(node, story); },
            openness: initOpenness,
            a1go1: initA1Go1,
            curves: initCurves
        };
        Array.prototype.slice.call(story.querySelectorAll(".qh-widget")).forEach(function (node) {
            var fn = widgets[node.dataset.widget];
            if (!fn) return;
            try {
                var ok = fn(node);
                if (ok !== false && node.childElementCount) {
                    var scene = sceneOf(node);
                    if (scene) scene.classList.add("qh-has-widget");
                }
            } catch (err) {
                /* 一个部件坏掉不能连累正文；没有 qh-has-widget 时回退图仍然显示。 */
                node.innerHTML = "";
                if (window.console && console.error) console.error("qh widget " + node.dataset.widget, err);
            }
        });

        try { initRail(story); } catch (err) { /* 时间尺是可选的 */ }
        initEntrance(story);

        var params = new URLSearchParams(window.location.search);
        if (params.has("highlight")) {
            Array.prototype.slice.call(story.querySelectorAll("details")).forEach(function (d) { d.open = true; });
        }
        window.addEventListener("beforeprint", function () {
            Array.prototype.slice.call(story.querySelectorAll("details")).forEach(function (d) { d.open = true; });
        });
    }

    if (document.readyState === "loading") {
        document.addEventListener("DOMContentLoaded", init);
    } else {
        init();
    }
})();
