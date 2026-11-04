/* 第 4 章 图 4.3：fatbin 浏览器。
 *
 * 页面里放 8 段工具原样输出（text/plain）：每种编译选项的 nvcc 命令、
 * cuobjdump --list-elf --list-ptx 和 ./tiny 的运行结果。这里解析出
 * 每种选项的 SASS / PTX 列表，再按正文 4.3 节的规则模拟驱动的选择。
 *
 * 纯函数（parseFatbin / parseArch / sassVerdict / ptxVerdict / conclude ...）
 * 不碰 DOM，Node 测试直接 require。
 */
(function () {
    "use strict";

    var DEFAULT_DEVICE = { major: 12, minor: 0 };

    /* ------------------------------------------------------------------ *
     * 纯函数
     * ------------------------------------------------------------------ */

    function normalizeText(text) {
        var value = String(text === null || text === undefined ? "" : text);
        value = value.replace(/\r\n?/g, "\n");
        if (value.charAt(0) === "\n") value = value.slice(1);
        if (value.charAt(value.length - 1) === "\n") value = value.slice(0, -1);
        return value;
    }

    /* "120" / "sm_120" / "compute_120" -> {digits, major, minor, sm, compute}。
       计算能力的两位写法是“主版本 + 一位次版本”。 */
    function parseArch(value) {
        var text = String(value === null || value === undefined ? "" : value).trim();
        var match = /(?:sm_|compute_)?(\d+)/i.exec(text);
        if (!match) return null;
        var digits = match[1];
        if (digits.length < 2) return null;
        var major = parseInt(digits.slice(0, -1), 10);
        var minor = parseInt(digits.slice(-1), 10);
        if (!isFinite(major) || !isFinite(minor)) return null;
        return {
            digits: digits,
            major: major,
            minor: minor,
            sm: "sm_" + digits,
            compute: "compute_" + digits
        };
    }

    function parseDevice(value) {
        if (value && typeof value === "object" && typeof value.major === "number") {
            return { major: value.major, minor: typeof value.minor === "number" ? value.minor : 0 };
        }
        var text = String(value === null || value === undefined ? "" : value).trim();
        var match = /(\d+)(?:\.(\d+))?/.exec(text);
        if (!match) return { major: DEFAULT_DEVICE.major, minor: DEFAULT_DEVICE.minor };
        return {
            major: parseInt(match[1], 10),
            minor: match[2] === undefined ? 0 : parseInt(match[2], 10)
        };
    }

    function archAtMost(arch, device) {
        if (!arch) return false;
        if (arch.major !== device.major) return arch.major < device.major;
        return arch.minor <= device.minor;
    }

    /* 单张 SASS 卡片在给定设备上的结论。 */
    function sassVerdict(value, device) {
        var arch = typeof value === "string" ? parseArch(value) : value;
        var dev = parseDevice(device);
        if (!arch) return { usable: false, label: "无法识别", arch: null };
        if (arch.major !== dev.major) {
            return { usable: false, label: "主版本不同", arch: arch };
        }
        if (arch.minor > dev.minor) {
            return { usable: false, label: "次版本更高", arch: arch };
        }
        return { usable: true, label: "能直接用", arch: arch };
    }

    /* 单张 PTX 卡片在给定设备上的结论；hasSass 时 PTX 用不上。 */
    function ptxVerdict(value, device, hasSass) {
        var arch = typeof value === "string" ? parseArch(value) : value;
        var dev = parseDevice(device);
        if (!arch) return { usable: false, label: "无法识别", arch: null };
        if (hasSass) return { usable: false, label: "用不上", arch: arch };
        if (archAtMost(arch, dev)) return { usable: true, label: "JIT 编译", arch: arch };
        return { usable: false, label: "计算能力更高", arch: arch };
    }

    /* 按“先 SASS，后 PTX”的顺序给出结论。sassArchs / ptxArchs 是 "120" 这样的字符串。 */
    function conclude(sassArchs, ptxArchs, device) {
        var dev = parseDevice(device);
        var sass = [];
        var ptx = [];
        var usableSass = [];
        var i;
        for (i = 0; i < (sassArchs || []).length; i++) {
            var sv = sassVerdict(sassArchs[i], dev);
            sass.push(sv);
            if (sv.usable) usableSass.push(sv.arch);
        }
        var hasSass = usableSass.length > 0;
        for (i = 0; i < (ptxArchs || []).length; i++) {
            var pv = ptxVerdict(ptxArchs[i], dev, hasSass);
            ptx.push(pv);
        }

        var result = { sass: sass, ptx: ptx, kind: "none", arch: null, conclusion: "" };
        if (hasSass) {
            var chosen = usableSass[0];
            for (i = 1; i < usableSass.length; i++) {
                if (usableSass[i].minor > chosen.minor) chosen = usableSass[i];
            }
            result.kind = "direct";
            result.arch = chosen;
            result.conclusion = "直接运行 " + chosen.sm + " 的 SASS";
            return result;
        }
        var usablePtx = [];
        for (i = 0; i < ptx.length; i++) if (ptx[i].usable) usablePtx.push(ptx[i].arch);
        if (usablePtx.length) {
            var best = usablePtx[0];
            for (i = 1; i < usablePtx.length; i++) {
                if (best === null || best.major < usablePtx[i].major ||
                    (best.major === usablePtx[i].major && best.minor < usablePtx[i].minor)) {
                    best = usablePtx[i];
                }
            }
            result.kind = "jit";
            result.arch = best;
            result.conclusion = "JIT 编译 " + best.compute + " 的 PTX，再运行";
            return result;
        }
        result.conclusion = "没有能用的代码：cudaErrorNoKernelImageForDevice";
        return result;
    }

    /* 结论和实际运行是否一致。 */
    function checkRun(result, run) {
        var expectedOk = result.kind !== "none";
        var actualOk = !!(run && run.ok);
        var consistent = expectedOk === actualOk;
        var message = "";
        if (!consistent) {
            message = "推出的结论和实际运行不一致（" + result.conclusion +
                "，实际：" + (run && run.text ? run.text : "无输出") + "）";
        }
        return { consistent: consistent, expectedOk: expectedOk, actualOk: actualOk, message: message };
    }

    /* 解析一段 ch04-fatbin-out。返回 [{option, group, command, raw, sass, ptx, run}]，
       sass 是 [{arch, elfs}]，ptx 是 [{arch}]，arch 是不带前缀的数字串。 */
    function parseFatbin(text) {
        var value = normalizeText(text);
        var chunks = value.split(/\n[ \t]*\n/);
        var out = [];
        for (var c = 0; c < chunks.length; c++) {
            var chunk = chunks[c].replace(/^\n+|\n+$/g, "");
            if (!chunk || !/^\$ nvcc\b/m.test(chunk)) continue;
            var lines = chunk.split("\n");
            var command = "";
            var option = "";
            for (var i = 0; i < lines.length; i++) {
                var cmd = /^\$\s*nvcc\s+(.+?)\s+tiny\.cu\s+-o\s+tiny\s*$/.exec(lines[i]);
                if (cmd) {
                    command = lines[i].replace(/^\$\s*/, "");
                    option = cmd[1].trim();
                    break;
                }
            }
            if (!option) continue;

            var sassByArch = {};
            var sassOrder = [];
            var ptxList = [];
            var run = { ok: false, text: "" };
            var runIndex = -1;
            for (var j = 0; j < lines.length; j++) {
                if (/^\$\s*\.\/tiny\s*$/.test(lines[j])) {
                    runIndex = j;
                    break;
                }
            }
            if (runIndex >= 0 && lines[runIndex + 1] !== undefined) {
                run.text = lines[runIndex + 1].trim();
                run.ok = /运行成功/.test(run.text);
            }

            for (var k = 0; k < lines.length; k++) {
                var elf = /^ELF file\s+\d+:\s+\S+\.sm_(\d+)\.cubin\s*$/.exec(lines[k]);
                if (elf) {
                    var arch = elf[1];
                    if (!sassByArch[arch]) {
                        sassByArch[arch] = { arch: arch, sm: "sm_" + arch, elfs: 0 };
                        sassOrder.push(arch);
                    }
                    sassByArch[arch].elfs += 1;
                    continue;
                }
                var ptx = /^PTX file\s+\d+:\s+\S+\.sm_(\d+)\.ptx\s*$/.exec(lines[k]);
                if (ptx) {
                    ptxList.push({ arch: ptx[1], compute: "compute_" + ptx[1] });
                }
            }

            var sass = [];
            for (var s = 0; s < sassOrder.length; s++) sass.push(sassByArch[sassOrder[s]]);
            out.push({
                option: option,
                group: /^-gencode\b/.test(option) ? "gencode" : "arch",
                command: command,
                raw: chunk,
                sass: sass,
                ptx: ptxList,
                run: run
            });
        }
        return out;
    }

    function sassArches(entry) {
        var out = [];
        for (var i = 0; i < entry.sass.length; i++) out.push(entry.sass[i].arch);
        return out;
    }

    function ptxArches(entry) {
        var out = [];
        for (var i = 0; i < entry.ptx.length; i++) out.push(entry.ptx[i].arch);
        return out;
    }

    /* 对一段解析结果给出结论和实际运行是否一致。 */
    function evaluate(entry, device) {
        var result = conclude(sassArches(entry), ptxArches(entry), device);
        var check = checkRun(result, entry.run);
        result.run = entry.run;
        result.check = check;
        return result;
    }

    /* ------------------------------------------------------------------ *
     * DOM
     * ------------------------------------------------------------------ */

    var contexts = [];
    var BIN_ANIM_STEP = 270;

    function warn(message, err) {
        if (typeof console === "undefined" || !console.warn) return;
        if (err) console.warn("[cu-fatbin] " + message, err);
        else console.warn("[cu-fatbin] " + message);
    }

    function el(tag, className, text) {
        var node = document.createElement(tag);
        if (className) node.className = className;
        if (text !== undefined && text !== null) node.textContent = String(text);
        return node;
    }

    function readText(id) {
        var node = document.getElementById(id);
        return node ? node.textContent : "";
    }

    function codeText(text) {
        var node = document.createElement("code");
        node.textContent = text;
        return node;
    }

    function makeButton(entry, selected) {
        var button = el("button", "cu-fatbin-option", entry.option);
        button.type = "button";
        button.setAttribute("data-option", entry.option);
        button.setAttribute("aria-pressed", selected ? "true" : "false");
        return button;
    }

    function cardType(sass) {
        return sass.elfs + " 个 ELF";
    }

    function setup(figure) {
        if (!figure || figure.getAttribute("data-cu-fatbin-ready") === "1") return null;
        figure.setAttribute("data-cu-fatbin-ready", "1");

        var sourceId = figure.getAttribute("data-source");
        var device = parseDevice(figure.getAttribute("data-device"));
        var entries = parseFatbin(readText(sourceId));
        if (!entries.length) {
            warn("没有读到图 4.3 的数据");
            return null;
        }

        var defaultIndex = 0;
        for (var d = 0; d < entries.length; d++) {
            if (entries[d].option === "-arch=sm_120") {
                defaultIndex = d;
                break;
            }
        }

        var ui = el("div", "cu-fatbin-ui");
        var controls = el("div", "cu-fatbin-controls");
        var groupBoxes = { arch: null, gencode: null };
        var groupLabels = { arch: "-arch 简写", gencode: "-gencode" };
        for (var g = 0; g < entries.length; g++) {
            var kind = entries[g].group;
            if (!groupBoxes[kind]) {
                var group = el("div", "cu-fatbin-group");
                group.setAttribute("data-group", kind);
                var label = el("span", "cu-fatbin-group-label");
                label.appendChild(codeText(groupLabels[kind]));
                var buttons = el("div", "cu-fatbin-buttons");
                group.appendChild(label);
                group.appendChild(buttons);
                controls.appendChild(group);
                groupBoxes[kind] = buttons;
            }
            groupBoxes[kind].appendChild(makeButton(entries[g], g === defaultIndex));
        }
        var replay = el("button", "cu-fatbin-replay", "重播");
        replay.type = "button";
        controls.appendChild(replay);
        ui.appendChild(controls);

        var cmd = el("div", "cu-fatbin-cmd");
        var cmdCode = el("code", "", entries[defaultIndex].command);
        cmd.appendChild(cmdCode);
        ui.appendChild(cmd);

        var stage = el("div", "cu-fatbin-stage");
        var bin = el("div", "cu-fatbin-bin");
        var binHead = el("div", "cu-fatbin-bin-head");
        binHead.appendChild(el("span", "cu-fatbin-bin-title", "fatbin"));
        binHead.appendChild(el("span", "cu-fatbin-bin-note", "可执行文件里装进去的东西"));
        bin.appendChild(binHead);
        var driver = el("div", "cu-fatbin-driver", "驱动");
        driver.setAttribute("aria-hidden", "true");
        bin.appendChild(driver);
        var grid = el("div", "cu-fatbin-grid");
        bin.appendChild(grid);
        stage.appendChild(bin);
        ui.appendChild(stage);

        var conclusion = el("div", "cu-fatbin-conclusion");
        conclusion.setAttribute("aria-live", "polite");
        var conclusionLabel = el("span", "cu-fatbin-conclusion-label", "结论");
        var conclusionText = el("strong", "cu-fatbin-conclusion-text", "");
        var actual = el("div", "cu-fatbin-actual");
        var warning = el("div", "cu-fatbin-warning");
        warning.hidden = true;
        conclusion.appendChild(conclusionLabel);
        conclusion.appendChild(conclusionText);
        conclusion.appendChild(actual);
        conclusion.appendChild(warning);
        ui.appendChild(conclusion);

        var rawDetails = el("details", "cu-fatbin-raw");
        var rawSummary = el("summary", "", "cuobjdump 原始输出");
        var rawPre = el("pre", "cu-fatbin-raw-pre");
        rawDetails.appendChild(rawSummary);
        rawDetails.appendChild(rawPre);
        ui.appendChild(rawDetails);

        var caption = figure.querySelector("figcaption");
        if (caption && caption.parentNode === figure) figure.insertBefore(ui, caption);
        else figure.appendChild(ui);

        var ctx = {
            figure: figure,
            ui: ui,
            entries: entries,
            device: device,
            controls: controls,
            buttons: figure.querySelectorAll(".cu-fatbin-option"),
            replay: replay,
            cmdCode: cmdCode,
            bin: bin,
            driver: driver,
            grid: grid,
            conclusion: conclusion,
            conclusionText: conclusionText,
            actual: actual,
            warning: warning,
            rawPre: rawPre,
            selected: defaultIndex,
            currentCard: null,
            timers: [],
            generation: 0
        };
        contexts.push(ctx);

        function clearTimers() {
            ctx.generation += 1;
            for (var i = 0; i < ctx.timers.length; i++) clearTimeout(ctx.timers[i]);
            ctx.timers = [];
        }

        function later(fn, ms) {
            var generation = ctx.generation;
            var id = setTimeout(function () {
                if (generation === ctx.generation) fn();
            }, ms);
            ctx.timers.push(id);
        }

        function setPressed(option) {
            for (var i = 0; i < ctx.buttons.length; i++) {
                ctx.buttons[i].setAttribute("aria-pressed",
                    ctx.buttons[i].getAttribute("data-option") === option ? "true" : "false");
            }
        }

        function buildCards(entry, result) {
            while (ctx.grid.firstChild) ctx.grid.removeChild(ctx.grid.firstChild);
            ctx.currentCard = null;
            var i;
            for (i = 0; i < entry.sass.length; i++) {
                var sass = entry.sass[i];
                var sassStatus = null;
                for (var s = 0; s < result.sass.length; s++) {
                    if (result.sass[s].arch && result.sass[s].arch.digits === sass.arch) sassStatus = result.sass[s];
                }
                var card = el("div", "cu-fatbin-card cu-fatbin-card-sass");
                card.setAttribute("data-kind", "sass");
                card.setAttribute("data-arch", sass.arch);
                if (sassStatus && sassStatus.usable) card.className += " is-good";
                else if (sassStatus && sassStatus.label === "次版本更高") card.className += " is-bad";
                else card.className += " is-bad";
                card.appendChild(el("span", "cu-fatbin-card-arch", "sm_" + sass.arch));
                card.appendChild(el("span", "cu-fatbin-card-type",
                    "SASS · " + cardType(sass)));
                var badge = el("span", "cu-fatbin-badge",
                    sassStatus ? sassStatus.label : "");
                card.appendChild(badge);
                card.setAttribute("aria-label",
                    "sm_" + sass.arch + " 的 SASS，" + sass.elfs + " 个 ELF，" +
                    (sassStatus ? sassStatus.label : ""));
                ctx.grid.appendChild(card);
            }
            for (i = 0; i < entry.ptx.length; i++) {
                var ptx = entry.ptx[i];
                var ptxStatus = null;
                for (var p = 0; p < result.ptx.length; p++) {
                    if (result.ptx[p].arch && result.ptx[p].arch.digits === ptx.arch) ptxStatus = result.ptx[p];
                }
                var pcard = el("div", "cu-fatbin-card cu-fatbin-card-ptx");
                pcard.setAttribute("data-kind", "ptx");
                pcard.setAttribute("data-arch", ptx.arch);
                if (ptxStatus && ptxStatus.usable) pcard.className += " is-jit";
                else if (ptxStatus && ptxStatus.label === "用不上") pcard.className += " is-idle";
                else pcard.className += " is-bad";
                pcard.appendChild(el("span", "cu-fatbin-card-arch", "compute_" + ptx.arch));
                pcard.appendChild(el("span", "cu-fatbin-card-type", "PTX · 还要再编译"));
                var pbadge = el("span", "cu-fatbin-badge", ptxStatus ? ptxStatus.label : "");
                pcard.appendChild(pbadge);
                pcard.setAttribute("aria-label",
                    "compute_" + ptx.arch + " 的 PTX，" + (ptxStatus ? ptxStatus.label : ""));
                ctx.grid.appendChild(pcard);
            }
        }

        function moveDriver(card) {
            if (!card) return;
            var binRect = ctx.bin.getBoundingClientRect();
            var cardRect = card.getBoundingClientRect();
            var x = cardRect.right - binRect.left - ctx.driver.offsetWidth - 6;
            var y = cardRect.top - binRect.top + 6;
            if (x < 4) x = 4;
            if (y < 4) y = 4;
            ctx.driver.style.transform = "translate(" + Math.round(x) + "px," + Math.round(y) + "px)";
        }

        function finishInstant(result) {
            var cards = ctx.grid.children;
            for (var i = 0; i < cards.length; i++) {
                cards[i].classList.remove("is-hidden", "is-scanning");
                cards[i].classList.add("is-scanned");
            }
            ctx.driver.classList.remove("is-visible");
            ctx.conclusion.classList.add("is-shown");
            showConclusion(result);
        }

        function showConclusion(result) {
            ctx.conclusionText.textContent = result.conclusion;
            ctx.conclusion.classList.remove("is-good", "is-jit", "is-none");
            ctx.conclusion.classList.add(result.kind === "direct" ? "is-good" :
                result.kind === "jit" ? "is-jit" : "is-none");
            ctx.actual.textContent = "实际运行：" + (result.run.text || "（无输出）");
            if (result.check.consistent) {
                ctx.warning.hidden = true;
                ctx.warning.textContent = "";
            } else {
                ctx.warning.hidden = false;
                ctx.warning.textContent = "⚠ " + result.check.message;
                if (typeof console !== "undefined" && console.error) {
                    console.error("[cu-fatbin] " + result.check.message);
                }
            }
        }

        function play(entry, result) {
            clearTimers();
            buildCards(entry, result);
            setPressed(entry.option);
            ctx.cmdCode.textContent = entry.command;
            ctx.rawPre.textContent = entry.raw;
            ctx.actual.textContent = "实际运行：" + (entry.run.text || "（无输出）");
            ctx.conclusionText.textContent = result.conclusion;
            ctx.warning.hidden = true;
            ctx.warning.textContent = "";
            if (result.check.consistent) {
                /* 动画结束再亮结论。 */
            } else {
                ctx.warning.hidden = false;
                ctx.warning.textContent = "⚠ " + result.check.message;
                if (typeof console !== "undefined" && console.error) {
                    console.error("[cu-fatbin] " + result.check.message);
                }
            }

            var reduced = typeof window !== "undefined" && window.matchMedia &&
                window.matchMedia("(prefers-reduced-motion: reduce)").matches;
            var cards = ctx.grid.children;
            var i;
            for (i = 0; i < cards.length; i++) {
                cards[i].classList.add("is-hidden");
                cards[i].classList.remove("is-scanned", "is-scanning");
            }
            ctx.driver.classList.remove("is-visible");
            ctx.conclusion.classList.remove("is-shown");
            ctx.actual.textContent = "实际运行：" + (entry.run.text || "（无输出）");

            if (reduced) {
                finishInstant(result);
                return;
            }

            var fade = 34;
            for (i = 0; i < cards.length; i++) {
                (function (card, index) {
                    later(function () { card.classList.remove("is-hidden"); }, 40 + index * fade);
                })(cards[i], i);
            }
            var scanStart = 40 + cards.length * fade + 120;
            var scanOrder = [];
            for (i = 0; i < cards.length; i++) scanOrder.push(cards[i]);
            later(function () { ctx.driver.classList.add("is-visible"); }, scanStart);
            for (i = 0; i < scanOrder.length; i++) {
                (function (card, index) {
                    var at = scanStart + index * BIN_ANIM_STEP;
                    later(function () {
                        for (var j = 0; j < scanOrder.length; j++) scanOrder[j].classList.remove("is-scanning");
                        card.classList.add("is-scanning");
                        ctx.currentCard = card;
                        moveDriver(card);
                    }, at);
                    later(function () {
                        card.classList.remove("is-scanning");
                        card.classList.add("is-scanned");
                    }, at + BIN_ANIM_STEP * 0.58);
                })(scanOrder[i], i);
            }
            later(function () {
                ctx.driver.classList.remove("is-visible");
                ctx.currentCard = null;
                ctx.conclusion.classList.add("is-shown");
                showConclusion(result);
            }, scanStart + scanOrder.length * BIN_ANIM_STEP + 80);
        }

        function select(index) {
            if (index < 0 || index >= ctx.entries.length) return;
            ctx.selected = index;
            play(ctx.entries[index], evaluate(ctx.entries[index], ctx.device));
        }

        for (var b = 0; b < ctx.buttons.length; b++) {
            ctx.buttons[b].addEventListener("click", function (event) {
                var option = event.currentTarget.getAttribute("data-option");
                for (var i = 0; i < ctx.entries.length; i++) {
                    if (ctx.entries[i].option === option) {
                        select(i);
                        return;
                    }
                }
            });
            // 左右键在选项之间移动焦点；mdBook 在 document 上用左右键翻章，所以要拦住冒泡。
            ctx.buttons[b].addEventListener("keydown", function (event) {
                if (event.key !== "ArrowLeft" && event.key !== "ArrowRight") return;
                event.preventDefault();
                event.stopPropagation();
                var list = Array.prototype.slice.call(ctx.buttons);
                var at = list.indexOf(event.currentTarget);
                var to = event.key === "ArrowRight" ? at + 1 : at - 1;
                if (to >= 0 && to < list.length) list[to].focus();
            });
        }
        replay.addEventListener("click", function () {
            select(ctx.selected);
        });
        window.addEventListener("resize", function () {
            if (ctx.currentCard) moveDriver(ctx.currentCard);
        });

        if (typeof window.matchMedia === "function") {
            var motion = window.matchMedia("(prefers-reduced-motion: reduce)");
            var onMotion = function () { if (motion.matches) select(ctx.selected); };
            if (motion.addEventListener) motion.addEventListener("change", onMotion);
            else if (motion.addListener) motion.addListener(onMotion);
        }

        select(defaultIndex);
        return ctx;
    }

    function init() {
        var roots = document.querySelectorAll(".cu-fatbin[data-source]");
        if (!roots.length) return;
        Array.prototype.forEach.call(roots, function (root) {
            try {
                setup(root);
            } catch (err) {
                warn("初始化失败", err);
            }
        });
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
            normalizeText: normalizeText,
            parseArch: parseArch,
            parseDevice: parseDevice,
            sassVerdict: sassVerdict,
            ptxVerdict: ptxVerdict,
            conclude: conclude,
            checkRun: checkRun,
            parseFatbin: parseFatbin,
            sassArches: sassArches,
            ptxArches: ptxArches,
            evaluate: evaluate
        };
    }
})();
