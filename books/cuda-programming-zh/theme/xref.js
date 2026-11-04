/* 第 4 章 图 4.2：源码 / PTX / SASS 对照。
 *
 * 页面里只放工具原样输出（text/plain），这里负责解析：
 *   - PTX：按 .loc 把指令归到源码行，并记住它在原文里的行号；
 *   - SASS：按地址注释取指令，从 -g 和 -gp 两份输出里取源码行、PTX 行；
 *   - 分步：figure 里的 application/json 只写 PTX 行号和说明。
 *
 * 纯函数（parseSource / parsePtx / parseSass / parseSteps ...）不碰 DOM，
 * 需要时由 Node 测试直接 require。
 */
(function () {
    "use strict";

    /* ------------------------------------------------------------------ *
     * 纯函数
     * ------------------------------------------------------------------ */

    /* 去掉 mdBook 展开时包在 script 内容外面的一个换行，统一换行符。 */
    function normalizeText(text) {
        var value = String(text === null || text === undefined ? "" : text);
        value = value.replace(/\r\n?/g, "\n");
        if (value.charAt(0) === "\n") value = value.slice(1);
        if (value.charAt(value.length - 1) === "\n") value = value.slice(0, -1);
        return value;
    }

    function splitLines(text) {
        var value = normalizeText(text);
        return value === "" ? [] : value.split("\n");
    }

    /* 源码：每行记下真实行号。 */
    function parseSource(text) {
        var lines = splitLines(text);
        var out = [];
        for (var i = 0; i < lines.length; i++) {
            out.push({ line: i + 1, text: lines[i] });
        }
        return out;
    }

    /* "17-20, 22" -> [{from,to}, ...]；只接受正整数。 */
    function parseRanges(spec) {
        var out = [];
        var parts = String(spec === null || spec === undefined ? "" : spec).split(",");
        for (var i = 0; i < parts.length; i++) {
            var part = parts[i].replace(/\s+/g, "");
            if (!part) continue;
            var match = /^(\d+)(?:-(\d+))?$/.exec(part);
            if (!match) continue;
            var from = parseInt(match[1], 10);
            var to = match[2] ? parseInt(match[2], 10) : from;
            if (!isFinite(from) || !isFinite(to) || from < 1 || to < from) continue;
            out.push({ from: from, to: to });
        }
        return out;
    }

    function sourceInRanges(lines, ranges) {
        var out = [];
        for (var i = 0; i < lines.length; i++) {
            for (var j = 0; j < ranges.length; j++) {
                if (lines[i].line >= ranges[j].from && lines[i].line <= ranges[j].to) {
                    out.push(lines[i]);
                    break;
                }
            }
        }
        return out;
    }

    /* 只显示 .visible .entry 到最后一个 }，去掉 .loc / .file / 空行 / 注释。
       .loc 之后的行（指令和 $L__BB0_2: 这样的标号）都归到那个源码行。 */
    function parsePtx(text) {
        var lines = splitLines(text);
        var entry = -1;
        var lastBrace = -1;
        for (var i = 0; i < lines.length; i++) {
            var trimmed = lines[i].trim();
            if (entry < 0 && /^\.visible\s+\.entry\b/.test(trimmed)) entry = i;
            if (trimmed === "}") lastBrace = i;
        }
        if (entry < 0 || lastBrace < entry) return [];

        var out = [];
        var loc = null;
        for (var j = entry; j <= lastBrace; j++) {
            var raw = lines[j];
            var line = raw.trim();
            if (!line) continue;
            if (/^\.loc\b/.test(line)) {
                var locMatch = /^\.loc\s+\d+\s+(\d+)/.exec(line);
                if (locMatch) loc = parseInt(locMatch[1], 10);
                continue;
            }
            if (/^\.file\b/.test(line)) continue;
            if (/^\/\//.test(line) || /^\/\*/.test(line) || /^\*/.test(line)) continue;
            out.push({
                line: j + 1,
                text: raw.replace(/^\s+/, ""),
                src: loc,
                label: /^\$[^\s:]+:/.test(line)
            });
        }
        return out;
    }

    /* 从一份 nvdisasm -g / -gp 输出里取“地址 -> 行号”。 */
    function parseSassMap(text) {
        var lines = splitLines(text);
        var map = {};
        var current = null;
        for (var i = 0; i < lines.length; i++) {
            var fileMatch = /\/\/##\s+File\s+"[^"]*",\s+line\s+(\d+)/.exec(lines[i]);
            if (fileMatch) {
                current = parseInt(fileMatch[1], 10);
                continue;
            }
            var insn = /\/\*([0-9a-fA-F]+)\*\/\s*(.*?)\s*$/.exec(lines[i]);
            if (insn && current !== null) {
                map[insn[1].toLowerCase()] = current;
            }
        }
        return map;
    }

    function isUnconditionalExit(text) {
        return !/^@/.test(text) && /(^|\s)EXIT\b/.test(text);
    }

    /* 解析 SASS。srcText 给源码行，ptxText 给 PTX 行；只到第一条无条件 EXIT。 */
    function parseSass(srcText, ptxText) {
        var srcMap = parseSassMap(srcText);
        var ptxMap = parseSassMap(ptxText);
        var lines = splitLines(srcText);
        var out = [];
        var current = null;
        for (var i = 0; i < lines.length; i++) {
            var fileMatch = /\/\/##\s+File\s+"[^"]*",\s+line\s+(\d+)/.exec(lines[i]);
            if (fileMatch) {
                current = parseInt(fileMatch[1], 10);
                continue;
            }
            var insn = /\/\*([0-9a-fA-F]+)\*\/\s*(.*?)\s*$/.exec(lines[i]);
            if (!insn) continue;
            var addr = insn[1].toLowerCase();
            var text = insn[2].trim();
            if (!text) continue;
            out.push({
                addr: addr,
                text: text,
                src: srcMap.hasOwnProperty(addr) ? srcMap[addr] : current,
                ptx: ptxMap.hasOwnProperty(addr) ? ptxMap[addr] : null
            });
            if (isUnconditionalExit(text)) break;
        }
        return out;
    }

    /* 分步配置：只接受 {ptx:[正整数], text:"说明"}。 */
    function parseSteps(value) {
        var data = value;
        if (typeof value === "string") {
            try {
                data = JSON.parse(value);
            } catch (err) {
                return [];
            }
        }
        if (!data || !data.steps || typeof data.steps.length !== "number") return [];
        var out = [];
        for (var i = 0; i < data.steps.length; i++) {
            var step = data.steps[i] || {};
            var ptx = [];
            if (step.ptx && typeof step.ptx.length === "number") {
                for (var j = 0; j < step.ptx.length; j++) {
                    var n = parseInt(step.ptx[j], 10);
                    if (isFinite(n) && n > 0) ptx.push(n);
                }
            }
            out.push({
                ptx: ptx,
                text: String(step.text === null || step.text === undefined ? "" : step.text)
            });
        }
        return out;
    }

    /* 一步对应的三栏集合。 */
    function stepSelection(step, ptxRows, sassRows) {
        var ptxSet = {};
        var srcSet = {};
        var sassSet = {};
        var byLine = {};
        for (var i = 0; i < ptxRows.length; i++) byLine[ptxRows[i].line] = ptxRows[i];
        for (var j = 0; j < step.ptx.length; j++) {
            var lineNo = step.ptx[j];
            ptxSet[lineNo] = true;
            var row = byLine[lineNo];
            if (row && row.src !== null && row.src !== undefined) srcSet[row.src] = true;
        }
        for (var k = 0; k < sassRows.length; k++) {
            if (ptxSet[sassRows[k].ptx]) sassSet[sassRows[k].addr] = true;
        }
        return { ptx: ptxSet, src: srcSet, sass: sassSet };
    }

    /* 检查每条 SASS 是否恰好被一步覆盖。返回 {covered, missing, duplicated}。 */
    function checkCoverage(steps, ptxRows, sassRows) {
        var count = {};
        var sassSet = {};
        for (var i = 0; i < sassRows.length; i++) sassSet[sassRows[i].addr] = true;
        for (var j = 0; j < steps.length; j++) {
            var selection = stepSelection(steps[j], ptxRows, sassRows);
            for (var addr in selection.sass) {
                if (!Object.prototype.hasOwnProperty.call(selection.sass, addr)) continue;
                count[addr] = (count[addr] || 0) + 1;
            }
        }
        var missing = [];
        var duplicated = [];
        for (var key in sassSet) {
            if (!Object.prototype.hasOwnProperty.call(sassSet, key)) continue;
            if (!count[key]) missing.push(key);
            else if (count[key] > 1) duplicated.push(key);
        }
        return {
            covered: sassRows.length - missing.length,
            total: sassRows.length,
            missing: missing,
            duplicated: duplicated
        };
    }

    /* ------------------------------------------------------------------ *
     * DOM
     * ------------------------------------------------------------------ */

    var contexts = [];
    var WIDE = "(min-width: 700px)";

    function warn(message, err) {
        if (typeof console === "undefined" || !console.warn) return;
        if (err) console.warn("[cu-xref] " + message, err);
        else console.warn("[cu-xref] " + message);
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

    function readJson(figure) {
        var script = figure.querySelector('script[type="application/json"]');
        if (!script) return null;
        try {
            return JSON.parse(script.textContent);
        } catch (err) {
            warn("JSON 解析失败", err);
            return null;
        }
    }

    /* 把说明里的 `code` 拆成节点，不用 innerHTML。 */
    function renderRichText(node, text) {
        var parts = String(text).split("`");
        for (var i = 0; i < parts.length; i++) {
            if (!parts[i]) continue;
            if (i % 2 === 1) node.appendChild(el("code", "", parts[i]));
            else node.appendChild(document.createTextNode(parts[i]));
        }
    }

    function setPressed(row, on) {
        if (on) row.setAttribute("aria-pressed", "true");
        else row.setAttribute("aria-pressed", "false");
    }

    function makeRow(kind, key, gutter, code, label) {
        var row = el("div", "cu-xref-row");
        row.setAttribute("data-kind", kind);
        row.setAttribute("data-key", String(key));
        row.setAttribute("tabindex", "0");
        row.setAttribute("role", "button");
        row.setAttribute("aria-pressed", "false");
        row.setAttribute("aria-label", label);
        row.appendChild(el("span", "cu-xref-gutter", gutter));
        row.appendChild(el("span", "cu-xref-code", code));
        return row;
    }

    function setup(figure) {
        if (!figure || figure.getAttribute("data-cu-xref-ready") === "1") return null;
        figure.setAttribute("data-cu-xref-ready", "1");

        var sourceId = figure.getAttribute("data-cu");
        var ptxId = figure.getAttribute("data-ptx");
        var sassSrcId = figure.getAttribute("data-sass-src");
        var sassPtxId = figure.getAttribute("data-sass-ptx");
        var ranges = parseRanges(figure.getAttribute("data-lines"));
        if (!ranges.length) ranges = parseRanges("17-20");

        var sourceAll = parseSource(readText(sourceId));
        var sourceRows = sourceInRanges(sourceAll, ranges);
        var ptxRows = parsePtx(readText(ptxId));
        var sassRows = parseSass(readText(sassSrcId), readText(sassPtxId));
        var steps = parseSteps(readJson(figure));

        if (!sourceRows.length && !ptxRows.length && !sassRows.length) {
            warn("没有读到图 4.2 的数据");
            return null;
        }

        for (var m = 0; m < sassRows.length; m++) {
            if (sassRows[m].src === null || sassRows[m].ptx === null) {
                warn("SASS " + sassRows[m].addr + " 缺少源码行或 PTX 行");
                break;
            }
        }
        if (steps.length) {
            var coverage = checkCoverage(steps, ptxRows, sassRows);
            if (coverage.missing.length || coverage.duplicated.length) {
                warn("分步没有恰好覆盖每条 SASS：" + JSON.stringify(coverage));
            }
        }

        var ptxByLine = {};
        for (var p = 0; p < ptxRows.length; p++) ptxByLine[ptxRows[p].line] = ptxRows[p];
        var sassByAddr = {};
        var sassByPtx = {};
        for (var s = 0; s < sassRows.length; s++) {
            sassByAddr[sassRows[s].addr] = sassRows[s];
            if (!sassByPtx[sassRows[s].ptx]) sassByPtx[sassRows[s].ptx] = [];
            sassByPtx[sassRows[s].ptx].push(sassRows[s].addr);
        }

        var ui = el("div", "cu-xref-ui");
        var toolbar = el("div", "cu-xref-toolbar");
        toolbar.setAttribute("role", "group");
        toolbar.setAttribute("aria-label", "分步查看对应关系");
        var prev = el("button", "cu-xref-btn", "上一步");
        var next = el("button", "cu-xref-btn", "下一步");
        var clear = el("button", "cu-xref-btn", "全部清除");
        prev.type = next.type = clear.type = "button";
        var stepCount = el("span", "cu-xref-step-count", "第 0 / " + steps.length + " 步");
        stepCount.setAttribute("aria-live", "polite");
        toolbar.appendChild(prev);
        toolbar.appendChild(next);
        toolbar.appendChild(clear);
        toolbar.appendChild(stepCount);
        ui.appendChild(toolbar);

        var stepText = el("p", "cu-xref-step-text",
            steps.length ? "点“下一步”从函数入口开始，或直接点任意一行。" : "直接点任意一行查看对应关系。");
        stepText.setAttribute("aria-live", "polite");
        ui.appendChild(stepText);

        var sourceHead = el("div", "cu-xref-col-head");
        sourceHead.appendChild(el("span", "cu-xref-col-title", "源码"));
        sourceHead.appendChild(el("span", "cu-xref-col-note",
            sourceRows.length ? "第 " + ranges[0].from + "–" + ranges[0].to + " 行" : ""));
        var sourcePane = el("div", "cu-xref-pane cu-xref-source");
        for (var i = 0; i < sourceRows.length; i++) {
            var srow = makeRow("source", sourceRows[i].line, String(sourceRows[i].line),
                sourceRows[i].text, "源码第 " + sourceRows[i].line + " 行");
            srow.setAttribute("data-line", String(sourceRows[i].line));
            sourcePane.appendChild(srow);
        }
        var sourceWrap = el("div", "cu-xref-source-wrap");
        sourceWrap.appendChild(sourceHead);
        sourceWrap.appendChild(sourcePane);
        ui.appendChild(sourceWrap);

        var body = el("div", "cu-xref-body");
        var svg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
        svg.setAttribute("class", "cu-xref-links");
        svg.setAttribute("aria-hidden", "true");
        body.appendChild(svg);

        var ptxSide = el("div", "cu-xref-side cu-xref-side-ptx");
        var ptxHead = el("div", "cu-xref-col-head");
        ptxHead.appendChild(el("span", "cu-xref-col-title", "PTX"));
        ptxHead.appendChild(el("span", "cu-xref-col-note", "原文行号"));
        var ptxPane = el("div", "cu-xref-pane cu-xref-ptx");
        for (var q = 0; q < ptxRows.length; q++) {
            var prow = ptxRows[q];
            var sassHere = sassByPtx[prow.line] || [];
            var plabel = "PTX 第 " + prow.line + " 行";
            if (prow.src !== null && prow.src !== undefined) plabel += "，对应源码第 " + prow.src + " 行";
            if (sassHere.length) plabel += "、SASS " + sassHere.join("、");
            var pnode = makeRow("ptx", prow.line, String(prow.line), prow.text, plabel);
            pnode.setAttribute("data-ptx-line", String(prow.line));
            ptxPane.appendChild(pnode);
        }
        ptxSide.appendChild(ptxHead);
        ptxSide.appendChild(ptxPane);
        body.appendChild(ptxSide);

        var sassSide = el("div", "cu-xref-side cu-xref-side-sass");
        var sassHead = el("div", "cu-xref-col-head");
        sassHead.appendChild(el("span", "cu-xref-col-title", "SASS"));
        sassHead.appendChild(el("span", "cu-xref-col-note", sassRows.length + " 条"));
        var sassPane = el("div", "cu-xref-pane cu-xref-sass");
        for (var r = 0; r < sassRows.length; r++) {
            var arow = sassRows[r];
            var alabel = "SASS " + arow.addr + "，对应源码第 " + arow.src + " 行、PTX 第 " + arow.ptx + " 行";
            var anode = makeRow("sass", arow.addr, arow.addr, arow.text, alabel);
            anode.setAttribute("data-sass-addr", arow.addr);
            sassPane.appendChild(anode);
        }
        sassSide.appendChild(sassHead);
        sassSide.appendChild(sassPane);
        body.appendChild(sassSide);
        ui.appendChild(body);

        var status = el("div", "cu-xref-status");
        status.setAttribute("role", "status");
        status.setAttribute("aria-live", "polite");
        status.textContent = "未选中任何行。";
        ui.appendChild(status);

        var caption = figure.querySelector("figcaption");
        var noscript = figure.querySelector("noscript");
        var anchor = noscript || caption;
        if (anchor && anchor.parentNode === figure) figure.insertBefore(ui, anchor);
        else figure.appendChild(ui);

        var ctx = {
            figure: figure,
            ui: ui,
            stepCount: stepCount,
            stepText: stepText,
            status: status,
            prev: prev,
            next: next,
            clear: clear,
            steps: steps,
            ptxRows: ptxRows,
            sassRows: sassRows,
            ptxByLine: ptxByLine,
            sassByAddr: sassByAddr,
            sassByPtx: sassByPtx,
            rows: figure.querySelectorAll(".cu-xref-row"),
            ptxPane: ptxPane,
            sassPane: sassPane,
            body: body,
            svg: svg,
            step: 0,
            selected: { source: {}, ptx: {}, sass: {} },
            wide: null,
            pendingFrame: false
        };
        contexts.push(ctx);

        function apply(sourceSet, ptxSet, sassSet, stepIndex) {
            ctx.step = stepIndex;
            ctx.selected = {
                source: sourceSet || {},
                ptx: ptxSet || {},
                sass: sassSet || {}
            };
            var any = false;
            for (var i = 0; i < ctx.rows.length; i++) {
                var row = ctx.rows[i];
                var kind = row.getAttribute("data-kind");
                var key = row.getAttribute("data-key");
                var on = false;
                if (kind === "source") on = !!ctx.selected.source[key];
                else if (kind === "ptx") on = !!ctx.selected.ptx[key];
                else if (kind === "sass") on = !!ctx.selected.sass[key];
                row.classList.toggle("is-on", on);
                setPressed(row, on);
                if (on) any = true;
            }
            figure.classList.toggle("is-selected", any);
            ctx.clear.disabled = !any && ctx.step === 0;
            ctx.prev.disabled = ctx.step <= 1;
            ctx.next.disabled = ctx.step >= ctx.steps.length || ctx.steps.length === 0;
            updateStatus(ctx);
            drawLinks(ctx);
        }

        function updateStep(index) {
            if (!ctx.steps.length) return;
            if (index < 1) {
                ctx.stepCount.textContent = "第 0 / " + ctx.steps.length + " 步";
                ctx.stepText.textContent = "点“下一步”从函数入口开始，或直接点任意一行。";
                apply({}, {}, {}, 0);
                return;
            }
            if (index > ctx.steps.length) index = ctx.steps.length;
            var step = ctx.steps[index - 1];
            var selection = stepSelection(step, ctx.ptxRows, ctx.sassRows);
            ctx.stepCount.textContent = "第 " + index + " / " + ctx.steps.length + " 步";
            ctx.stepText.textContent = "";
            renderRichText(ctx.stepText, step.text);
            apply(selection.src, selection.ptx, selection.sass, index);
        }

        function select(kind, key) {
            if (kind === "source") {
                var line = parseInt(key, 10);
                var srcSet = {};
                var ptxSet = {};
                var sassSet = {};
                srcSet[line] = true;
                for (var i = 0; i < ctx.ptxRows.length; i++) {
                    var prow = ctx.ptxRows[i];
                    if (prow.src === line) ptxSet[prow.line] = true;
                }
                for (var j = 0; j < ctx.sassRows.length; j++) {
                    if (ctx.sassRows[j].src === line) sassSet[ctx.sassRows[j].addr] = true;
                }
                ctx.stepCount.textContent = "第 0 / " + ctx.steps.length + " 步";
                ctx.stepText.textContent = "点“下一步”按 " + ctx.steps.length + " 步走，或继续点任意一行。";
                apply(srcSet, ptxSet, sassSet, 0);
                return;
            }
            if (kind === "ptx") {
                var ptxLine = parseInt(key, 10);
                var row = ctx.ptxByLine[ptxLine];
                var ptxOne = {};
                var srcOne = {};
                var sassOne = {};
                ptxOne[ptxLine] = true;
                if (row && row.src !== null && row.src !== undefined) srcOne[row.src] = true;
                for (var k = 0; k < ctx.sassRows.length; k++) {
                    if (ctx.sassRows[k].ptx === ptxLine) sassOne[ctx.sassRows[k].addr] = true;
                }
                ctx.stepCount.textContent = "第 0 / " + ctx.steps.length + " 步";
                ctx.stepText.textContent = "点“下一步”按 " + ctx.steps.length + " 步走，或继续点任意一行。";
                apply(srcOne, ptxOne, sassOne, 0);
                return;
            }
            var sassRow = ctx.sassByAddr[key];
            if (!sassRow) return;
            var sassKey = {};
            var ptxKey = {};
            var srcKey = {};
            sassKey[sassRow.addr] = true;
            ptxKey[sassRow.ptx] = true;
            srcKey[sassRow.src] = true;
            ctx.stepCount.textContent = "第 0 / " + ctx.steps.length + " 步";
            ctx.stepText.textContent = "点“下一步”按 " + ctx.steps.length + " 步走，或继续点任意一行。";
            apply(srcKey, ptxKey, sassKey, 0);
        }

        function onRowKey(event) {
            var row = event.currentTarget;
            if (event.key === "Enter" || event.key === " " || event.key === "Spacebar") {
                event.preventDefault();
                event.stopPropagation();
                select(row.getAttribute("data-kind"), row.getAttribute("data-key"));
                return;
            }
            if (event.key === "ArrowLeft" || event.key === "ArrowRight") {
                // mdBook 在 document 上用左右键翻章；这里只拦住冒泡，保留栏内横向滚动。
                event.stopPropagation();
                return;
            }
            if (event.key === "ArrowDown" || event.key === "ArrowUp") {
                event.preventDefault();
                event.stopPropagation();
                var pane = row.parentNode;
                var rows = pane ? pane.querySelectorAll(".cu-xref-row") : [];
                var at = -1;
                for (var i = 0; i < rows.length; i++) if (rows[i] === row) at = i;
                if (at < 0) return;
                var to = event.key === "ArrowDown" ? at + 1 : at - 1;
                if (to >= 0 && to < rows.length) rows[to].focus();
            }
        }

        for (var z = 0; z < ctx.rows.length; z++) {
            ctx.rows[z].addEventListener("click", function (event) {
                var row = event.currentTarget;
                select(row.getAttribute("data-kind"), row.getAttribute("data-key"));
            });
            ctx.rows[z].addEventListener("keydown", onRowKey);
        }
        prev.addEventListener("click", function () {
            if (ctx.step <= 1) return;
            updateStep(ctx.step - 1);
        });
        next.addEventListener("click", function () {
            updateStep(Math.min(ctx.step + 1, ctx.steps.length));
        });
        clear.addEventListener("click", function () {
            ctx.step = 0;
            if (ctx.steps.length) {
                ctx.stepCount.textContent = "第 0 / " + ctx.steps.length + " 步";
                ctx.stepText.textContent = "点“下一步”从函数入口开始，或直接点任意一行。";
            }
            apply({}, {}, {}, 0);
        });

        if (typeof window.matchMedia === "function") {
            ctx.wide = window.matchMedia(WIDE);
            var onMode = function () { drawLinks(ctx); };
            if (ctx.wide.addEventListener) ctx.wide.addEventListener("change", onMode);
            else if (ctx.wide.addListener) ctx.wide.addListener(onMode);
        }

        window.addEventListener("resize", function () {
            scheduleLinks(ctx);
        });
        if (typeof ResizeObserver === "function") {
            new ResizeObserver(function () { scheduleLinks(ctx); }).observe(ctx.body);
        }
        if (document.fonts && document.fonts.ready && document.fonts.ready.then) {
            document.fonts.ready.then(function () { scheduleLinks(ctx); });
        }

        apply({}, {}, {}, 0);
        return ctx;
    }

    function updateStatus(ctx) {
        var src = Object.keys(ctx.selected.source);
        var ptx = Object.keys(ctx.selected.ptx);
        var sass = Object.keys(ctx.selected.sass);
        if (!src.length && !ptx.length && !sass.length) {
            ctx.status.textContent = "未选中任何行。";
            return;
        }
        var parts = [];
        if (src.length) parts.push("源码第 " + src.join("、") + " 行");
        if (ptx.length) parts.push("PTX 第 " + ptx.join("、") + " 行");
        if (sass.length) parts.push("SASS " + sass.join("、"));
        ctx.status.textContent = "已选中：" + parts.join("；") + "。";
    }

    /* 两条栏之间画当前 SASS 到它 PTX 行的连线。窄屏不画。 */
    function drawLinks(ctx) {
        var svg = ctx.svg;
        while (svg.firstChild) svg.removeChild(svg.firstChild);
        if (ctx.wide && !ctx.wide.matches) return;
        if (typeof window === "undefined") return;

        var bodyRect = ctx.body.getBoundingClientRect();
        var ptxRect = ctx.ptxPane.getBoundingClientRect();
        var sassRect = ctx.sassPane.getBoundingClientRect();
        var width = Math.max(0, Math.round(bodyRect.width));
        var height = Math.max(0, Math.round(bodyRect.height));
        if (!width || !height) return;
        svg.setAttribute("width", String(width));
        svg.setAttribute("height", String(height));
        svg.setAttribute("viewBox", "0 0 " + width + " " + height);

        var sassKeys = Object.keys(ctx.selected.sass);
        for (var i = 0; i < sassKeys.length; i++) {
            var sassRow = ctx.sassByAddr[sassKeys[i]];
            if (!sassRow) continue;
            var ptxRow = ctx.ptxByLine[sassRow.ptx];
            if (!ptxRow || !ctx.selected.ptx[sassRow.ptx]) continue;
            var ptxNode = null;
            var sassNode = null;
            var rows = ctx.rows;
            for (var j = 0; j < rows.length; j++) {
                if (rows[j].getAttribute("data-kind") === "ptx" &&
                    rows[j].getAttribute("data-key") === String(sassRow.ptx)) ptxNode = rows[j];
                if (rows[j].getAttribute("data-kind") === "sass" &&
                    rows[j].getAttribute("data-key") === sassRow.addr) sassNode = rows[j];
            }
            if (!ptxNode || !sassNode) continue;
            var r1 = ptxNode.getBoundingClientRect();
            var r2 = sassNode.getBoundingClientRect();
            var x1 = ptxRect.right - bodyRect.left + 4;
            var x2 = sassRect.left - bodyRect.left - 4;
            if (x2 <= x1) continue;
            var y1 = r1.top + r1.height / 2 - bodyRect.top;
            var y2 = r2.top + r2.height / 2 - bodyRect.top;
            var bend = Math.max(12, (x2 - x1) * 0.42);
            var path = document.createElementNS("http://www.w3.org/2000/svg", "path");
            path.setAttribute("class", "cu-xref-link");
            path.setAttribute("d",
                "M" + x1.toFixed(1) + "," + y1.toFixed(1) +
                " C" + (x1 + bend).toFixed(1) + "," + y1.toFixed(1) +
                " " + (x2 - bend).toFixed(1) + "," + y2.toFixed(1) +
                " " + x2.toFixed(1) + "," + y2.toFixed(1));
            path.setAttribute("pathLength", "1");
            svg.appendChild(path);
        }
    }

    function scheduleLinks(ctx) {
        if (ctx.pendingFrame) return;
        ctx.pendingFrame = true;
        var raf = typeof window.requestAnimationFrame === "function"
            ? window.requestAnimationFrame.bind(window)
            : function (fn) { return setTimeout(fn, 16); };
        raf(function () {
            ctx.pendingFrame = false;
            drawLinks(ctx);
        });
    }

    function init() {
        var roots = document.querySelectorAll(".cu-xref[data-cu]");
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
            splitLines: splitLines,
            parseSource: parseSource,
            parseRanges: parseRanges,
            sourceInRanges: sourceInRanges,
            parsePtx: parsePtx,
            parseSassMap: parseSassMap,
            parseSass: parseSass,
            isUnconditionalExit: isUnconditionalExit,
            parseSteps: parseSteps,
            stepSelection: stepSelection,
            checkCoverage: checkCoverage
        };
    }
})();
