/* 第 2 章 vecadd 六步动画。
 *
 * 纯函数 parseLaps / sumLaps / stateAfter 不碰 DOM，可以在 Node 里直接测；
 * 其余部分只在页面存在 .cu-steps 时运行。没有 JS 时 .cu-step-stage 为空、
 * 不占高度，正文和代码照常显示。
 *
 * 这个文件不引用其他书的 theme，也不依赖 charts.js / charts.css。
 */
(function () {
    "use strict";

    var STEPS = ["host", "malloc", "h2d", "kernel", "d2h", "free"];
    var SHORT = {
        host: "准备输入",
        malloc: "分配显存",
        h2d: "拷进显存",
        kernel: "启动 kernel",
        d2h: "拷回、检查",
        free: "释放"
    };
    var LAP_RE = /^\s*(\d+(?:\.\d+)?)\s*(ms|us|s)\s{2,}(.+?)\s*$/;
    var REDUCED = typeof window !== "undefined" &&
        typeof window.matchMedia === "function" &&
        window.matchMedia("(prefers-reduced-motion: reduce)").matches;

    /* ------------------------------------------------------------------ *
     * 纯函数
     * ------------------------------------------------------------------ */

    function parseLaps(text) {
        var lines = String(text === null || text === undefined ? "" : text).split(/\r?\n/);
        var laps = [];
        for (var i = 0; i < lines.length; i++) {
            var m = LAP_RE.exec(lines[i]);
            if (m) {
                laps.push({
                    value: Number(m[1]),
                    text: m[1],
                    unit: m[2],
                    label: m[3]
                });
            }
        }
        if (!laps.length) throw new Error("parseLaps: 没有找到计时行");
        return laps;
    }

    function trimZeros(text) {
        return text.replace(/(\.\d*?)0+$/, "$1").replace(/\.$/, "");
    }

    function sumLaps(laps, names) {
        var wanted = String(names === null || names === undefined ? "" : names).split("|");
        var matches = [];
        for (var i = 0; i < laps.length; i++) {
            for (var j = 0; j < wanted.length; j++) {
                if (laps[i].label === wanted[j]) {
                    matches.push(laps[i]);
                    break;
                }
            }
        }
        if (!matches.length) {
            throw new Error("sumLaps: 找不到 lap " + String(names));
        }
        var total = 0;
        for (var k = 0; k < matches.length; k++) total += matches[k].value;
        total = Math.round(total * 1e9) / 1e9;
        var text = matches.length === 1 ? matches[0].text : trimZeros(total.toFixed(3));
        return { value: total, text: text };
    }

    /* null = 不存在；"empty" = 已分配但空；"data" = 有数据。 */
    function blankState() {
        return {
            step: null,
            host: { h_a: null, h_b: null, h_c: null },
            device: { d_a: null, d_b: null, d_c: null },
            context: false,
            pointers: false,
            threads: false
        };
    }

    function stateAfter(step) {
        var index = STEPS.indexOf(step);
        if (index < 0) throw new Error("stateAfter: 未知步骤 " + String(step));
        var state = blankState();
        state.step = step;

        state.host.h_a = "data";
        state.host.h_b = "data";
        state.host.h_c = "empty";

        if (index >= 1) {
            state.device.d_a = "empty";
            state.device.d_b = "empty";
            state.device.d_c = "empty";
            state.context = true;
            state.pointers = true;
        }
        if (index >= 2) {
            state.device.d_a = "data";
            state.device.d_b = "data";
        }
        if (index >= 3) {
            state.device.d_c = "data";
            state.threads = true;
        }
        if (index >= 4) {
            state.host.h_c = "data";
        }
        if (index >= 5) {
            state.host = { h_a: null, h_b: null, h_c: null };
            state.device = { d_a: null, d_b: null, d_c: null };
            state.context = false;
            state.pointers = false;
            state.threads = false;
        }
        return state;
    }

    /* ------------------------------------------------------------------ *
     * DOM 小工具
     * ------------------------------------------------------------------ */

    var NS = "http://www.w3.org/2000/svg";
    var contexts = [];
    var framePending = false;
    var UID = 0;

    function warn(message, err) {
        if (typeof console === "undefined" || !console.warn) return;
        if (err) console.warn("[cu-steps] " + message, err);
        else console.warn("[cu-steps] " + message);
    }

    function el(tag, cls, text) {
        var node = document.createElement(tag);
        if (cls) node.className = cls;
        if (text !== undefined) node.textContent = text;
        return node;
    }

    function svgEl(tag, cls, attrs) {
        var node = document.createElementNS(NS, tag);
        if (cls) node.setAttribute("class", cls);
        if (attrs) {
            for (var key in attrs) {
                if (Object.prototype.hasOwnProperty.call(attrs, key)) {
                    node.setAttribute(key, String(attrs[key]));
                }
            }
        }
        return node;
    }

    function svgText(parent, x, y, text, cls) {
        var node = svgEl("text", cls, { x: x, y: y });
        node.textContent = text;
        parent.appendChild(node);
        return node;
    }

    function menuHeight() {
        var value = parseFloat(getComputedStyle(document.documentElement)
            .getPropertyValue("--menu-bar-height"));
        return isFinite(value) && value > 0 ? value : 50;
    }

    function formatTime(value) {
        if (value >= 10) return value.toFixed(1);
        var text = value.toPrecision(3);
        if (text.indexOf("e") >= 0 || text.indexOf("E") >= 0) return String(value);
        return trimZeros(text);
    }

    function lastLine(text) {
        var lines = String(text || "").split(/\r?\n/);
        for (var i = lines.length - 1; i >= 0; i--) {
            var line = lines[i].trim();
            if (line) return line;
        }
        return "";
    }

    /* ------------------------------------------------------------------ *
     * 数据解析
     * ------------------------------------------------------------------ */

    function readData(root) {
        var sourceId = root.getAttribute("data-source");
        var source = sourceId ? document.getElementById(sourceId) : null;
        var text = source ? (source.textContent || "") : "";
        var laps = [];
        if (!source) {
            warn("找不到数据源 #" + String(sourceId));
        } else {
            try {
                laps = parseLaps(text);
            } catch (err) {
                warn("数据源 #" + sourceId + " 里没有计时行", err);
            }
        }
        var check = lastLine(text);
        if (check && LAP_RE.test(check)) check = "";
        return { laps: laps, check: check, hasSource: !!source };
    }

    function sectionTitle(section) {
        var node = section.querySelector(".cu-step-title");
        return node ? (node.textContent || "").trim() : "";
    }

    /* ------------------------------------------------------------------ *
     * 初始化
     * ------------------------------------------------------------------ */

    function setupRoot(root) {
        if (root.getAttribute("data-cu-ready") === "1") return null;
        root.setAttribute("data-cu-ready", "1");

        var rc = {
            root: root,
            entries: [],
            rail: null,
            current: -1,
            reduced: REDUCED,
            lastTop: null,
            lastMargin: null,
            playObserver: null,
            revealObserver: null,
            resizeObserver: null
        };
        var content = root.closest ? root.closest(".content") : null;
        if (content) content.style.overflowY = "visible";

        var data = readData(root);
        var sections = Array.prototype.slice.call(root.querySelectorAll(".cu-step"));

        sections.forEach(function (section) {
            var step = section.getAttribute("data-step") || "";
            var index = STEPS.indexOf(step);
            if (index < 0) {
                warn("未知的 data-step：" + step);
                return;
            }
            var stage = section.querySelector(".cu-step-stage");
            if (!stage) {
                warn("第 " + (index + 1) + " 步缺少 .cu-step-stage");
                return;
            }
            var entry = {
                rc: rc,
                section: section,
                stage: stage,
                step: step,
                index: index,
                prevStep: index > 0 ? STEPS[index - 1] : null,
                fromState: index > 0 ? stateAfter(STEPS[index - 1]) : blankState(),
                finalState: stateAfter(step),
                disabled: false,
                played: false,
                replaying: false,
                reduced: rc.reduced,
                canAnimate: typeof Element !== "undefined" &&
                    typeof Element.prototype.animate === "function",
                check: data.check,
                state: null,
                displayStep: null,
                layout: null,
                lastWidth: 0,
                uid: ++UID,
                animations: [],
                timers: [],
                refs: null,
                geo: null,
                button: null,
                svg: null
            };

            var lapNames = section.getAttribute("data-laps");
            if (lapNames) {
                try {
                    var timing = sumLaps(data.laps, lapNames);
                    var badge = section.querySelector(".cu-step-badge");
                    var time = el("span", "cu-step-time", formatTime(timing.value) + " ms");
                    time.setAttribute("title", "实测 " + timing.text + " ms");
                    if (badge && badge.parentNode) {
                        badge.parentNode.insertBefore(time, badge.nextSibling);
                    } else {
                        section.insertBefore(time, section.firstChild);
                    }
                } catch (err) {
                    entry.disabled = true;
                    warn("第 " + (index + 1) + " 步的 lap 对不上：" + lapNames, err);
                }
            }
            rc.entries.push(entry);
        });

        if (!rc.entries.length) return rc;

        buildRail(rc);

        rc.entries.forEach(function (entry) {
            if (entry.disabled) return;
            var width = Math.round(entry.stage.clientWidth || root.clientWidth || 600);
            entry.lastWidth = width;
            try {
                drawStage(entry,
                    rc.reduced ? entry.finalState : entry.fromState,
                    rc.reduced ? entry.step : entry.prevStep,
                    { replay: false });
                if (rc.reduced) entry.played = true;
            } catch (err) {
                warn("第 " + (entry.index + 1) + " 步的示意图绘制失败", err);
            }
        });

        initReveal(rc);

        if (typeof IntersectionObserver === "function") {
            try {
                rc.playObserver = new IntersectionObserver(function () {
                    scheduleFrame();
                }, { threshold: [0, 0.25, 0.5, 0.75, 1] });
                rc.entries.forEach(function (entry) {
                    if (!entry.disabled) rc.playObserver.observe(entry.section);
                });
            } catch (err) {
                rc.playObserver = null;
            }
        }

        if (typeof ResizeObserver === "function") {
            try {
                rc.resizeObserver = new ResizeObserver(function () {
                    scheduleFrame();
                });
                rc.entries.forEach(function (entry) {
                    if (!entry.disabled) rc.resizeObserver.observe(entry.stage);
                });
            } catch (err) {
                rc.resizeObserver = null;
            }
        }

        setCurrent(rc, 0);
        rc.entries.forEach(function (entry) {
            if (!entry.disabled) revealIfNeeded(rc, entry);
        });
        syncChrome(rc);
        return rc;
    }

    function buildRail(rc) {
        var nav = el("nav", "cu-steps-rail");
        nav.setAttribute("aria-label", "步骤进度");
        var inner = el("div", "cu-steps-rail-inner");

        rc.entries.forEach(function (entry, i) {
            var button = el("button", "cu-steps-rail-btn");
            button.type = "button";
            var num = el("span", "cu-rail-num", String(entry.index + 1));
            var name = el("span", "cu-rail-name", SHORT[entry.step] || entry.step);
            var tick = el("span", "cu-rail-tick", "✓");
            tick.setAttribute("aria-hidden", "true");
            button.appendChild(num);
            button.appendChild(name);
            button.appendChild(tick);
            button.setAttribute("aria-label",
                "跳到第 " + (entry.index + 1) + " 步：" + (sectionTitle(entry.section) || name.textContent));
            button.addEventListener("click", function () {
                scrollToEntry(rc, i);
            });
            entry.button = button;
            inner.appendChild(button);
        });

        nav.appendChild(inner);
        rc.root.insertBefore(nav, rc.root.firstChild);
        rc.rail = nav;
    }

    function setCurrent(rc, index) {
        if (index < 0 || index >= rc.entries.length) return;
        rc.current = index;
        rc.entries.forEach(function (entry, i) {
            if (!entry.button) return;
            entry.button.classList.toggle("cu-on", i === index);
            entry.button.classList.toggle("cu-done", i < index);
            if (i === index) entry.button.setAttribute("aria-current", "step");
            else entry.button.removeAttribute("aria-current");
        });
        if (rc.rail) {
            var pct = ((index + 1) / rc.entries.length) * 100;
            rc.rail.style.setProperty("--cu-progress", pct.toFixed(2) + "%");
        }
    }

    function scrollToEntry(rc, index) {
        var entry = rc.entries[index];
        if (!entry) return;
        var railHeight = rc.rail ? rc.rail.offsetHeight : 0;
        var top = entry.section.getBoundingClientRect().top +
            (window.pageYOffset || document.documentElement.scrollTop || 0);
        var goingUp = entry.section.getBoundingClientRect().top < 0;
        var offset = railHeight + 10 + (goingUp ? menuHeight() : 0);
        if (!rc.reduced && typeof window.scrollTo === "function") {
            window.scrollTo({ top: Math.max(0, top - offset), behavior: "smooth" });
        } else {
            window.scrollTo(0, Math.max(0, top - offset));
        }
        setCurrent(rc, index);
    }

    /* ------------------------------------------------------------------ *
     * 布局
     * ------------------------------------------------------------------ */

    function geometryFor(width, layout) {
        var cellH = 22;
        var cellsInset = 44;
        var cellGap = 1;
        var host;
        var device;
        var hostRows;
        var deviceRows;
        var corridor;
        var pcieLabel;
        var hostStatusY;
        var deviceStatusY;
        var noteY;
        var threadY;
        var threadLabelY;
        var tagsY = 52;
        var contextY;

        if (layout === "wide") {
            var margin = 10;
            var gap = 72;
            var panelW = Math.max(150, (width - margin * 2 - gap) / 2);
            host = { x: margin, y: 8, w: panelW, h: 284 };
            device = { x: margin + panelW + gap, y: 8, w: panelW, h: 284 };
            hostRows = [100, 136, 172];
            deviceRows = [100, 136, 172];
            corridor = { x: host.x + host.w + 6, y: 94, w: gap - 12, h: 104 };
            pcieLabel = { x: host.x + host.w + gap / 2, y: 214 };
            hostStatusY = 276;
            deviceStatusY = 276;
            noteY = 88;
            threadY = 90;
            threadLabelY = 84;
            contextY = 14;
        } else {
            var left = 26;
            var right = 10;
            var narrowW = Math.max(120, width - left - right);
            host = { x: left, y: 8, w: narrowW, h: 214 };
            device = { x: left, y: 300, w: narrowW, h: 236 };
            hostRows = [108, 142, 176];
            deviceRows = [390, 424, 458];
            corridor = { x: left, y: 226, w: narrowW, h: 70 };
            pcieLabel = { x: left + 8, y: 265 };
            hostStatusY = 212;
            deviceStatusY = 522;
            noteY = 98;
            threadY = 380;
            threadLabelY = 372;
            contextY = 314;
        }

        var cellsEnd = host.x + host.w - 10;
        var cellsX = host.x + cellsInset;
        var cellW = Math.max(3, (cellsEnd - cellsX - 15 * cellGap) / 16);

        return {
            layout: layout,
            width: width,
            cellH: cellH,
            cellGap: cellGap,
            cellW: cellW,
            cellsX: cellsX,
            host: host,
            device: device,
            hostRows: hostRows,
            deviceRows: deviceRows,
            corridor: corridor,
            pcieLabel: pcieLabel,
            hostStatusY: hostStatusY,
            deviceStatusY: deviceStatusY,
            noteY: noteY,
            threadY: threadY,
            threadLabelY: threadLabelY,
            tagsY: tagsY,
            context: {
                x: device.x + device.w - 66,
                y: contextY,
                w: 56,
                h: 16
            }
        };
    }

    /* ------------------------------------------------------------------ *
     * 绘制
     * ------------------------------------------------------------------ */

    function cellX(cellsX, geo, index) {
        return cellsX + index * (geo.cellW + geo.cellGap);
    }

    function makeArray(parent, refs, geo, panel, rows, name, value, series) {
        var group = svgEl("g", "cu-array", { "data-array": name });
        var rowY = rows[STEPS_HINT[name]];
        var centers = [];
        var fills = [];
        var cellsX = panel.x + 44;

        svgText(group, panel.x + 10, rowY + geo.cellH / 2 + 4, name, "cu-array-name");

        for (var i = 0; i < 16; i++) {
            var x = cellX(cellsX, geo, i);
            group.appendChild(svgEl("rect", "cu-cell", {
                x: x, y: rowY, width: geo.cellW, height: geo.cellH, rx: 2
            }));
            if (value === "data") {
                var fill = svgEl("rect", "cu-fill " + series, {
                    x: x + 1, y: rowY + 1,
                    width: Math.max(1, geo.cellW - 2),
                    height: geo.cellH - 2,
                    rx: 1.5
                });
                group.appendChild(fill);
                fills.push(fill);
            }
            centers.push({
                x: x + geo.cellW / 2,
                y: rowY + geo.cellH / 2
            });
        }

        parent.appendChild(group);
        refs.groups[name] = group;
        refs.fills[name] = fills;
        refs.centers[name] = centers;
    }

    var STEPS_HINT = { h_a: 0, h_b: 1, h_c: 2, d_a: 0, d_b: 1, d_c: 2 };
    var SERIES = ["cu-s1", "cu-s2", "cu-s3"];

    function makePointer(parent, refs, geo) {
        var group = svgEl("g", "cu-pointer-group");
        var tags = svgEl("g", "cu-pointer-tags");
        var chipW = 28;
        var chipsStart = geo.host.x + geo.host.w - 10 - (3 * chipW + 8);
        var chipH = 16;
        var chipGap = 4;
        var deviceKeys = ["d_a", "d_b", "d_c"];

        svgText(tags, geo.host.x + 10, geo.tagsY + 11.5, "主机上的变量", "cu-tag-label");
        svgText(tags, geo.host.x + 10, geo.noteY, "主机代码不能直接读", "cu-note");

        var starts = [];
        deviceKeys.forEach(function (name, i) {
            var x = chipsStart + i * (chipW + chipGap);
            tags.appendChild(svgEl("rect", "cu-tag", {
                x: x, y: geo.tagsY, width: chipW, height: chipH, rx: 3
            }));
            var label = svgText(tags, x + chipW / 2, geo.tagsY + 11.5, name, "cu-tag-text");
            label.setAttribute("text-anchor", "middle");
            starts.push({ x: x + chipW / 2, y: geo.tagsY + chipH });
        });

        group.appendChild(tags);
        refs.pointerTags = tags;

        var paths = [];
        deviceKeys.forEach(function (name, i) {
            var rowCenter = geo.deviceRows[i] + geo.cellH / 2;
            var lane;
            var exitY;
            if (geo.layout === "wide") {
                lane = geo.host.x + geo.host.w + 10 + i * 8;
                exitY = 72 + i * 2;
            } else {
                lane = geo.host.x - 6 - i * 6;
                exitY = 72 + i * 4;
            }
            var d = "M" + starts[i].x + "," + starts[i].y +
                " V" + exitY +
                " H" + lane +
                " V" + rowCenter +
                " H" + (geo.device.x + 7);
            var path = svgEl("path", "cu-pointer-line", { d: d });
            path.setAttribute("marker-end", "url(#cu-arrow-" + refs.uid + ")");
            group.appendChild(path);
            paths.push(path);
        });

        parent.appendChild(group);
        refs.pointerGroup = group;
        refs.pointerPaths = paths;
    }

    function makeThreads(parent, refs, geo) {
        var group = svgEl("g", "cu-thread-group");
        svgText(group, geo.device.x + 10, geo.threadLabelY, "每个元素一个线程", "cu-thread-label");
        var dots = [];
        var centers = refs.centers.d_c || [];
        for (var i = 0; i < centers.length; i++) {
            var dot = svgEl("circle", "cu-thread", {
                cx: centers[i].x,
                cy: geo.threadY,
                r: 3
            });
            group.appendChild(dot);
            dots.push(dot);
        }
        parent.appendChild(group);
        refs.threadGroup = group;
        refs.threadDots = dots;
    }

    function describeArrays(map, keys) {
        var data = [];
        var empty = [];
        keys.forEach(function (key) {
            if (map[key] === "data") data.push(key);
            else if (map[key] === "empty") empty.push(key);
        });
        var parts = [];
        if (data.length) parts.push(data.join("、") + " 已有数据");
        if (empty.length) parts.push(empty.join("、") + " 已分配");
        return parts.join("，");
    }

    function ariaFor(state) {
        var host = describeArrays(state.host, ["h_a", "h_b", "h_c"]);
        var device = describeArrays(state.device, ["d_a", "d_b", "d_c"]);
        if (!host && !device) {
            return "示意图：主机内存和显存里都没有数组，两块面板只剩标题。";
        }
        var text = "示意图：主机内存里 " + (host || "没有数组") + "；";
        if (device) {
            text += "显存里 " + device;
            if (state.context) text += "，context 已建立";
            if (state.pointers) text += "，主机上的变量指向显存";
            text += "。";
        } else {
            text += "显存里还没有数组。";
        }
        return text;
    }

    function statusFor(step, entry) {
        var hostStatus = "malloc + 填数：sin²(i)、cos²(i)";
        switch (step) {
        case "host":
            return { host: hostStatus, device: "" };
        case "malloc":
            return { host: "d_a、d_b、d_c 拿到显存地址", device: "第一个 CUDA 调用：建立 context" };
        case "h2d":
            return { host: "h_a、h_b 经 PCIe 送出", device: "cudaMemcpy：主机 -> 显存" };
        case "kernel":
            return {
                host: "cudaDeviceSynchronize：等 GPU 做完",
                device: "add<<<blocks, threads>>>"
            };
        case "d2h":
            return {
                host: entry.check ? "检查：" + entry.check + " ✓" : "检查结果",
                device: "cudaMemcpy：显存 -> 主机"
            };
        default:
            return { host: "", device: "" };
        }
    }

    function buildScene(entry, width, height, geo, state, stepKey) {
        var refs = {
            uid: entry.uid,
            groups: {},
            fills: {},
            centers: {},
            pointerGroup: null,
            pointerTags: null,
            pointerPaths: [],
            threadGroup: null,
            threadDots: [],
            corridor: null,
            statusHost: null,
            statusDevice: null,
            packetLayer: null
        };
        var svg = svgEl("svg", "cu-steps-svg", {
            width: width,
            height: height,
            viewBox: "0 0 " + width + " " + height,
            role: "img",
            focusable: "false",
            "aria-label": ariaFor(state)
        });

        var defs = svgEl("defs");
        var marker = svgEl("marker", null, {
            id: "cu-arrow-" + entry.uid,
            markerWidth: 7,
            markerHeight: 7,
            refX: 6,
            refY: 3.5,
            orient: "auto",
            markerUnits: "userSpaceOnUse"
        });
        marker.appendChild(svgEl("path", "cu-arrow-head", { d: "M0,0 L7,3.5 L0,7 Z" }));
        defs.appendChild(marker);
        svg.appendChild(defs);

        [geo.host, geo.device].forEach(function (panel) {
            svg.appendChild(svgEl("rect", "cu-panel", {
                x: panel.x, y: panel.y, width: panel.w, height: panel.h, rx: 8
            }));
        });

        var corridor = svgEl("rect", "cu-pcie", {
            x: geo.corridor.x,
            y: geo.corridor.y,
            width: geo.corridor.w,
            height: geo.corridor.h,
            rx: 6
        });
        svg.appendChild(corridor);
        refs.corridor = corridor;
        var pcie = svgText(svg, geo.pcieLabel.x, geo.pcieLabel.y, "PCIe", "cu-pcie-label");
        if (geo.layout === "wide") pcie.setAttribute("text-anchor", "middle");

        svgText(svg, geo.host.x + 10, geo.host.y + 22, "主机内存", "cu-panel-name");
        svgText(svg, geo.host.x + 10, geo.host.y + 39, "CPU 读写", "cu-panel-sub");
        svgText(svg, geo.device.x + 10, geo.device.y + 22, "显存", "cu-panel-name");
        svgText(svg, geo.device.x + 10, geo.device.y + 39, "GPU 读写", "cu-panel-sub");

        if (state.context) {
            var contextGroup = svgEl("g", "cu-context-group");
            contextGroup.appendChild(svgEl("rect", "cu-context", {
                x: geo.context.x, y: geo.context.y,
                width: geo.context.w, height: geo.context.h, rx: 3
            }));
            var ctxText = svgText(contextGroup,
                geo.context.x + geo.context.w / 2,
                geo.context.y + 11.5,
                "context", "cu-context-text");
            ctxText.setAttribute("text-anchor", "middle");
            svg.appendChild(contextGroup);
            refs.contextGroup = contextGroup;
        }

        var hostKeys = ["h_a", "h_b", "h_c"];
        var deviceKeys = ["d_a", "d_b", "d_c"];
        hostKeys.forEach(function (name, i) {
            if (!state.host[name]) return;
            makeArray(svg, refs, geo, geo.host, geo.hostRows, name, state.host[name], SERIES[i]);
        });
        deviceKeys.forEach(function (name, i) {
            if (!state.device[name]) return;
            makeArray(svg, refs, geo, geo.device, geo.deviceRows, name, state.device[name], SERIES[i]);
        });

        if (state.pointers) makePointer(svg, refs, geo);
        if (state.threads) makeThreads(svg, refs, geo);

        var status = statusFor(stepKey, entry);
        if (status.host) {
            refs.statusHost = svgText(svg,
                geo.host.x + 10, geo.hostStatusY, status.host, "cu-status");
        }
        if (status.device) {
            refs.statusDevice = svgText(svg,
                geo.device.x + 10, geo.deviceStatusY, status.device, "cu-status");
        }

        refs.packetLayer = svgEl("g", "cu-packets");
        svg.appendChild(refs.packetLayer);

        return { svg: svg, refs: refs };
    }

    function heightFor(layout) {
        return layout === "wide" ? 320 : 570;
    }

    function cancelEntry(entry) {
        for (var i = 0; i < entry.animations.length; i++) {
            try {
                entry.animations[i].cancel();
            } catch (err) {
                /* 已经结束的动画不用管 */
            }
        }
        entry.animations = [];
        for (var j = 0; j < entry.timers.length; j++) clearTimeout(entry.timers[j]);
        entry.timers = [];
    }

    function drawStage(entry, state, stepKey, options) {
        options = options || {};
        cancelEntry(entry);

        var rawWidth = entry.stage.clientWidth || entry.rc.root.clientWidth || 600;
        var width = Math.max(220, Math.round(rawWidth));
        var layout = width >= 560 ? "wide" : "narrow";
        var height = heightFor(layout);

        entry.stage.style.height = height + "px";
        entry.stage.classList.toggle("cu-stage-wide", layout === "wide");
        entry.stage.classList.toggle("cu-stage-narrow", layout !== "wide");
        entry.stage.textContent = "";

        var geo = geometryFor(width, layout);
        var scene = buildScene(entry, width, height, geo, state, stepKey);
        entry.stage.appendChild(scene.svg);

        entry.svg = scene.svg;
        entry.refs = scene.refs;
        entry.geo = geo;
        entry.layout = layout;
        entry.state = state;
        entry.displayStep = stepKey;
        entry.lastWidth = Math.round(entry.stage.clientWidth) || width;

        if (!entry.reduced && entry.canAnimate && options.replay !== false) {
            var button = el("button", "cu-step-replay", "重播");
            button.type = "button";
            button.setAttribute("aria-label", "重播这一步的动画");
            button.addEventListener("click", function () {
                replayEntry(entry);
            });
            entry.stage.appendChild(button);
        }
    }

    /* ------------------------------------------------------------------ *
     * 动画
     * ------------------------------------------------------------------ */

    function anim(entry, node, frames, options) {
        if (!node || !entry.canAnimate) return null;
        var opts = { duration: 240, fill: "both", easing: "ease" };
        if (options) {
            for (var key in options) {
                if (Object.prototype.hasOwnProperty.call(options, key)) opts[key] = options[key];
            }
        }
        try {
            var animation = node.animate(frames, opts);
            entry.animations.push(animation);
            return animation;
        } catch (err) {
            return null;
        }
    }

    function fade(entry, node, delay, duration, from, to) {
        return anim(entry, node, [
            { opacity: from },
            { opacity: to }
        ], { delay: delay, duration: duration, easing: "ease" });
    }

    function later(entry, fn, ms) {
        var id = setTimeout(function () {
            try {
                fn();
            } catch (err) {
                warn("动画回调失败", err);
            }
        }, ms);
        entry.timers.push(id);
        return id;
    }

    function animateHost(entry) {
        ["h_a", "h_b", "h_c"].forEach(function (name, i) {
            fade(entry, entry.refs.groups[name], i * 130, 240, 0, 1);
        });
        [["h_a", 280], ["h_b", 920]].forEach(function (pair) {
            var fills = entry.refs.fills[pair[0]] || [];
            fills.forEach(function (fill, i) {
                fade(entry, fill, pair[1] + i * 32, 110, 0, 1);
            });
        });
    }

    function animateMalloc(entry) {
        fade(entry, entry.refs.contextGroup, 80, 300, 0, 1);
        ["d_a", "d_b", "d_c"].forEach(function (name, i) {
            fade(entry, entry.refs.groups[name], 430 + i * 130, 260, 0, 1);
        });
        fade(entry, entry.refs.pointerTags, 900, 240, 0, 1);
        (entry.refs.pointerPaths || []).forEach(function (path, i) {
            fade(entry, path, 940 + i * 70, 240, 0, 1);
        });
    }

    function packetAt(entry, from, to, series, delay, duration, done) {
        var size = Math.max(5, Math.min(9, entry.geo.cellW * 0.7));
        var packet = svgEl("rect", "cu-packet " + series, {
            x: from.x - size / 2,
            y: from.y - size / 2,
            width: size,
            height: size,
            rx: 1.5
        });
        packet.style.opacity = "0";
        entry.refs.packetLayer.appendChild(packet);

        var dx = to.x - from.x;
        var dy = to.y - from.y;
        anim(entry, packet, [
            { transform: "translate(0px, 0px)", opacity: 1 },
            { transform: "translate(" + dx + "px, " + dy + "px)", opacity: 1 }
        ], {
            delay: delay,
            duration: duration,
            easing: "linear",
            fill: "forwards"
        });
        later(entry, function () {
            if (packet.parentNode) packet.parentNode.removeChild(packet);
            if (done) done();
        }, delay + duration + 2);
    }

    function animateH2D(entry) {
        var total = 100 + 32 * 50 + 100;
        entry.refs.corridor.classList.add("cu-pcie-active");
        later(entry, function () {
            entry.refs.corridor.classList.remove("cu-pcie-active");
        }, total);

        [["a", "cu-s1"], ["b", "cu-s2"]].forEach(function (pair, arrayIndex) {
            var channel = pair[0];
            var series = pair[1];
            for (var i = 0; i < 16; i++) {
                (function (slot) {
                    var index = arrayIndex * 16 + slot;
                    var delay = 100 + index * 50;
                    var from = entry.refs.centers["h_" + channel][slot];
                    var to = entry.refs.centers["d_" + channel][slot];
                    var fill = (entry.refs.fills["d_" + channel] || [])[slot];
                    packetAt(entry, from, to, series, delay, 50);
                    if (fill) fade(entry, fill, delay + 38, 80, 0, 1);
                })(i);
            }
        });
    }

    function animateD2H(entry) {
        var total = 100 + 16 * 65 + 120;
        entry.refs.corridor.classList.add("cu-pcie-active");
        later(entry, function () {
            entry.refs.corridor.classList.remove("cu-pcie-active");
        }, total);

        if (entry.refs.statusHost) {
            entry.refs.statusHost.textContent = "cudaDeviceSynchronize：等 GPU 做完";
        }

        for (var i = 0; i < 16; i++) {
            (function (slot) {
                var delay = 100 + slot * 65;
                var from = entry.refs.centers.d_c[slot];
                var to = entry.refs.centers.h_c[slot];
                var fill = (entry.refs.fills.h_c || [])[slot];
                packetAt(entry, from, to, "cu-s3", delay, 65);
                if (fill) fade(entry, fill, delay + 50, 90, 0, 1);
            })(i);
        }

        later(entry, function () {
            if (entry.refs.statusHost) {
                var status = statusFor("d2h", entry);
                entry.refs.statusHost.textContent = status.host;
            }
        }, total);
    }

    function animateKernel(entry) {
        fade(entry, entry.refs.threadGroup, 0, 180, 0, 1);
        var fills = entry.refs.fills.d_c || [];
        entry.refs.threadDots.forEach(function (dot, i) {
            var delay = Math.random() * 150;
            fade(entry, dot, delay, 260, 0.22, 1);
            if (fills[i]) fade(entry, fills[i], delay, 300, 0, 1);
        });
        if (entry.refs.statusHost) entry.refs.statusHost.textContent = "启动后立刻返回";
        later(entry, function () {
            if (entry.refs.statusHost && entry.displayStep === "kernel") {
                entry.refs.statusHost.textContent = statusFor("kernel", entry).host;
            }
        }, 1200);
    }

    function animateFree(entry) {
        var deviceNodes = ["d_a", "d_b", "d_c"].map(function (name) {
            return entry.refs.groups[name];
        }).filter(Boolean);
        [entry.refs.pointerGroup, entry.refs.contextGroup, entry.refs.threadGroup].forEach(function (node) {
            if (node) deviceNodes.push(node);
        });
        deviceNodes.forEach(function (node) {
            fade(entry, node, 100, 420, 1, 0);
        });
        if (entry.refs.statusDevice) entry.refs.statusDevice.textContent = "cudaFree";

        later(entry, function () {
            if (entry.refs.statusHost) entry.refs.statusHost.textContent = "free";
            ["h_a", "h_b", "h_c"].forEach(function (name) {
                fade(entry, entry.refs.groups[name], 0, 420, 1, 0);
            });
        }, 560);

        later(entry, function () {
            drawStage(entry, entry.finalState, "free", { replay: true });
        }, 1220);
    }

    function runStep(entry, animate) {
        if (entry.disabled) {
            entry.played = true;
            return;
        }
        var finalState = entry.finalState;
        entry.replaying = false;

        if (!animate || entry.reduced || !entry.canAnimate) {
            drawStage(entry, finalState, entry.step, { replay: false });
            entry.played = true;
            return;
        }

        if (entry.step === "free") {
            drawStage(entry, entry.fromState, entry.prevStep, { replay: false });
            entry.state = entry.finalState;
            entry.displayStep = "free";
            entry.played = true;
            animateFree(entry);
            return;
        }

        drawStage(entry, finalState, entry.step, { replay: true });
        entry.played = true;

        switch (entry.step) {
        case "host":
            animateHost(entry);
            break;
        case "malloc":
            animateMalloc(entry);
            break;
        case "h2d":
            animateH2D(entry);
            break;
        case "kernel":
            animateKernel(entry);
            break;
        case "d2h":
            animateD2H(entry);
            break;
        default:
            break;
        }
    }

    function replayEntry(entry) {
        if (entry.disabled || entry.reduced || !entry.canAnimate || entry.replaying) return;
        entry.replaying = true;
        drawStage(entry, entry.fromState, entry.prevStep, { replay: false });
        entry.state = entry.finalState;
        entry.displayStep = entry.step;
        later(entry, function () {
            entry.replaying = false;
            runStep(entry, true);
        }, 200);
    }

    /* ------------------------------------------------------------------ *
     * 滚动、吸顶和入场
     * ------------------------------------------------------------------ */

    function syncChrome(rc) {
        var menu = document.getElementById("mdbook-menu-bar");
        var height = menuHeight();
        var top = height;
        if (menu) {
            var rect = menu.getBoundingClientRect();
            top = Math.max(0, Math.min(height, rect.bottom));
        }
        top = Math.round(top);
        if (rc.lastTop !== top) {
            rc.root.style.setProperty("--cu-rail-top", top + "px");
            rc.lastTop = top;
        }

        var railHeight = rc.rail ? rc.rail.offsetHeight : 0;
        var margin = Math.round(top + railHeight + 8);
        if (rc.lastMargin !== margin) {
            rc.entries.forEach(function (entry) {
                entry.section.style.scrollMarginTop = margin + "px";
            });
            rc.lastMargin = margin;
        }
    }

    function visibleRatio(rect, topLimit) {
        var vh = window.innerHeight || document.documentElement.clientHeight;
        var top = Math.max(rect.top, topLimit);
        var bottom = Math.min(rect.bottom, vh);
        var visible = Math.max(0, bottom - top);
        var base = Math.min(rect.height, Math.max(1, vh - topLimit));
        return visible / base;
    }

    function updateCurrent(rc) {
        if (!rc.entries.length) return;
        var vh = window.innerHeight || document.documentElement.clientHeight;
        var topLimit = 0;
        if (rc.rail) {
            var railRect = rc.rail.getBoundingClientRect();
            topLimit = Math.max(0, Math.min(vh * 0.5, railRect.bottom));
        }

        var best = -1;
        var bestTop = Infinity;
        rc.entries.forEach(function (entry, i) {
            var rect = entry.section.getBoundingClientRect();
            if (rect.height <= 0) return;
            if (visibleRatio(rect, topLimit) >= 0.5 && rect.top < bestTop) {
                bestTop = rect.top;
                best = i;
            }
        });

        if (best < 0) {
            rc.entries.forEach(function (entry, i) {
                var rect = entry.section.getBoundingClientRect();
                if (rect.top <= vh * 0.45 && rect.bottom > 0) best = i;
            });
        }
        if (best < 0) best = rc.current >= 0 ? rc.current : 0;
        if (best !== rc.current) setCurrent(rc, best);
    }

    function visibleEnough(entry) {
        var rect = entry.section.getBoundingClientRect();
        if (rect.height <= 0) return false;
        var vh = window.innerHeight || document.documentElement.clientHeight;
        var visible = Math.min(rect.bottom, vh) - Math.max(rect.top, 0);
        if (visible <= 0) return false;
        return visible / Math.min(rect.height, vh) >= 0.5;
    }

    function maybePlay(entry) {
        if (entry.disabled || entry.played) return;
        if (!visibleEnough(entry)) return;
        try {
            runStep(entry, true);
        } catch (err) {
            warn("第 " + (entry.index + 1) + " 步动画失败", err);
            entry.played = true;
            try {
                drawStage(entry, entry.finalState, entry.step, { replay: false });
            } catch (err2) {
                /* 这一步不显示示意图 */
            }
        }
    }

    function initReveal(rc) {
        if (rc.reduced || typeof IntersectionObserver !== "function") {
            rc.entries.forEach(function (entry) {
                entry.section.classList.add("cu-step-in");
            });
            return;
        }
        rc.entries.forEach(function (entry) {
            entry.section.setAttribute("data-cu-reveal", "");
        });
        try {
            rc.revealObserver = new IntersectionObserver(function (entries) {
                entries.forEach(function (item) {
                    if (!item.isIntersecting) return;
                    item.target.classList.add("cu-step-in");
                    rc.revealObserver.unobserve(item.target);
                });
            }, { rootMargin: "0px 0px -8% 0px", threshold: 0.02 });
            rc.entries.forEach(function (entry) {
                rc.revealObserver.observe(entry.section);
            });
        } catch (err) {
            rc.entries.forEach(function (entry) {
                entry.section.classList.add("cu-step-in");
            });
        }
        setTimeout(function () {
            rc.entries.forEach(function (entry) {
                revealIfNeeded(rc, entry);
            });
        }, 3000);
    }

    function revealIfNeeded(rc, entry) {
        if (rc.reduced) return;
        if (entry.section.classList.contains("cu-step-in")) return;
        var rect = entry.section.getBoundingClientRect();
        var vh = window.innerHeight || document.documentElement.clientHeight;
        if (rect.top < vh * 1.05 && rect.bottom > -200) {
            entry.section.classList.add("cu-step-in");
        }
    }

    function redrawEntry(entry) {
        entry.replaying = false;
        try {
            if (entry.played) {
                drawStage(entry, entry.state, entry.displayStep, { replay: true });
            } else {
                drawStage(entry, entry.fromState, entry.prevStep, { replay: false });
            }
        } catch (err) {
            warn("第 " + (entry.index + 1) + " 步重绘失败", err);
        }
    }

    function tick() {
        for (var i = 0; i < contexts.length; i++) {
            var rc = contexts[i];
            syncChrome(rc);
            updateCurrent(rc);
            for (var j = 0; j < rc.entries.length; j++) {
                var entry = rc.entries[j];
                if (entry.disabled) continue;
                revealIfNeeded(rc, entry);
                var width = Math.round(entry.stage.clientWidth);
                if (width > 0 && Math.abs(width - entry.lastWidth) >= 1) {
                    entry.lastWidth = width;
                    redrawEntry(entry);
                } else {
                    maybePlay(entry);
                }
            }
        }
    }

    function scheduleFrame() {
        if (framePending) return;
        framePending = true;
        var raf = typeof window.requestAnimationFrame === "function"
            ? window.requestAnimationFrame.bind(window)
            : function (fn) { return setTimeout(fn, 16); };
        raf(function () {
            framePending = false;
            tick();
        });
    }

    function beforePrint() {
        contexts.forEach(function (rc) {
            rc.entries.forEach(function (entry) {
                if (entry.disabled) return;
                entry.replaying = false;
                try {
                    drawStage(entry, entry.finalState, entry.step, { replay: false });
                } catch (err) {
                    /* 打印时尽力而为 */
                }
            });
        });
    }

    function afterPrint() {
        contexts.forEach(function (rc) {
            rc.entries.forEach(function (entry) {
                if (entry.disabled) return;
                try {
                    if (entry.played) {
                        drawStage(entry, entry.finalState, entry.step, { replay: true });
                    } else {
                        drawStage(entry, entry.fromState, entry.prevStep, { replay: false });
                    }
                } catch (err) {
                    /* 忽略 */
                }
            });
        });
    }

    function init() {
        var roots = document.querySelectorAll(".cu-steps");
        if (!roots.length) return;
        Array.prototype.forEach.call(roots, function (root) {
            try {
                var rc = setupRoot(root);
                if (rc) contexts.push(rc);
            } catch (err) {
                warn("初始化失败", err);
            }
        });
        if (!contexts.length) return;

        window.addEventListener("scroll", scheduleFrame, { passive: true });
        window.addEventListener("resize", scheduleFrame);
        window.addEventListener("load", scheduleFrame);
        document.addEventListener("scroll", scheduleFrame, { passive: true, capture: true });
        window.addEventListener("beforeprint", beforePrint);
        window.addEventListener("afterprint", afterPrint);
        scheduleFrame();
    }

    if (typeof document !== "undefined") {
        if (document.readyState === "loading") {
            document.addEventListener("DOMContentLoaded", init);
        } else {
            init();
        }
    }

    if (typeof module === "object" && module.exports) {
        module.exports = {
            parseLaps: parseLaps,
            sumLaps: sumLaps,
            stateAfter: stateAfter
        };
    }
})();
