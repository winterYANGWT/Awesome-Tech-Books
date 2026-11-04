/* 第 4 章 图 4.2 / 图 4.3：源码 → PTX → SASS 对照。
 *
 * 页面里只放工具原样输出（text/plain），这里按 <figure data-mode> 分两种模式：
 *   - cu-ptx：源码和 PTX 上下排，按 .loc 给 PTX 着色；
 *   - ptx-sass：PTX 函数体和 SASS 左右排，中间画对应连线。
 *
 * 解析部分（parseSource / parsePtx / parseSass / stepSelection / checkCoverage ...）
 * 不碰 DOM，需要时由 Node 测试直接 require。
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

    /* 图 4.3 只显示 PTX 函数体：从 { 到最后的 }，去掉 .reg 声明。
       .entry / .param 在图 4.2 已经看过，这里不再占地方；去掉的行也不参与对应。 */
    function parsePtxBody(text) {
        var rows = parsePtx(text);
        var start = -1;
        var end = -1;
        var i;
        for (i = 0; i < rows.length; i++) {
            if (rows[i].text.trim() === "{") {
                start = i;
                break;
            }
        }
        for (i = rows.length - 1; i >= 0; i--) {
            if (rows[i].text.trim() === "}") {
                end = i;
                break;
            }
        }
        if (start < 0 || end < start) return [];
        var out = [];
        for (i = start; i <= end; i++) {
            if (/^\.reg\b/.test(rows[i].text.trim())) continue;
            out.push(rows[i]);
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

    function positiveInts(value) {
        var out = [];
        if (!value || typeof value.length !== "number") return out;
        for (var i = 0; i < value.length; i++) {
            var n = parseInt(value[i], 10);
            if (isFinite(n) && n > 0) out.push(n);
        }
        return out;
    }

    /* 分步配置：{ptx:[正整数], cu:[正整数], label:"短标签"}。
       text 是旧字段，仍兼容，统一放进 label 和 text 两个属性。 */
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
            var label = step.label !== undefined && step.label !== null
                ? step.label : step.text;
            label = String(label === null || label === undefined ? "" : label);
            out.push({
                ptx: positiveInts(step.ptx),
                cu: positiveInts(step.cu),
                label: label,
                text: label
            });
        }
        return out;
    }

    /* 图 4.2 的一步：源码行集合 + 这些源码行对应的全部 PTX 行。 */
    function sourceStepSelection(step, ptxRows) {
        var sourceSet = {};
        var ptxSet = {};
        var i;
        for (i = 0; i < step.cu.length; i++) sourceSet[step.cu[i]] = true;
        for (i = 0; i < ptxRows.length; i++) {
            if (sourceSet[ptxRows[i].src]) ptxSet[ptxRows[i].line] = true;
        }
        return { source: sourceSet, ptx: ptxSet };
    }

    /* 图 4.3 的一步：PTX 行集合 + 这些 PTX 行对应的 SASS。 */
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

    function makeHead(title, note) {
        var head = el("div", "cu-xref-col-head");
        head.appendChild(el("span", "cu-xref-col-title", title));
        if (note) head.appendChild(el("span", "cu-xref-col-note", note));
        return head;
    }

    function defaultStepText(ctx) {
        if (!ctx.steps.length) return "直接点任意一行查看对应关系。";
        if (ctx.mode === "cu-ptx") {
            return "点“下一步”按 " + ctx.steps.length + " 步看源码各行，或直接点任意一行。";
        }
        return "点“下一步”从函数入口开始，或直接点任意一行。";
    }

    function setStepZero(ctx) {
        ctx.step = 0;
        if (ctx.steps.length) {
            ctx.stepCount.textContent = "第 0 / " + ctx.steps.length + " 步";
        }
        ctx.stepText.textContent = defaultStepText(ctx);
        apply(ctx, {}, {}, {}, 0);
    }

    function setup(figure) {
        if (!figure || figure.getAttribute("data-cu-xref-ready") === "1") return null;
        figure.setAttribute("data-cu-xref-ready", "1");

        var mode = figure.getAttribute("data-mode");
        if (mode !== "cu-ptx" && mode !== "ptx-sass") {
            mode = figure.getAttribute("data-sass-src") ? "ptx-sass" : "cu-ptx";
        }

        var sourceRows = [];
        var ptxRows = [];
        var sassRows = [];
        var ranges = [];
        var sourceAll = [];

        if (mode === "cu-ptx") {
            ranges = parseRanges(figure.getAttribute("data-lines"));
            if (!ranges.length) ranges = parseRanges("17-20");
            sourceAll = parseSource(readText(figure.getAttribute("data-cu")));
            sourceRows = sourceInRanges(sourceAll, ranges);
            ptxRows = parsePtx(readText(figure.getAttribute("data-ptx")));
        } else {
            ptxRows = parsePtxBody(readText(figure.getAttribute("data-ptx")));
            sassRows = parseSass(readText(figure.getAttribute("data-sass-src")),
                readText(figure.getAttribute("data-sass-ptx")));
        }
        var steps = parseSteps(readJson(figure));

        if (!sourceRows.length && !ptxRows.length && !sassRows.length) {
            warn("没有读到 " + mode + " 的数据");
            return null;
        }

        if (mode === "ptx-sass") {
            var j;
            for (j = 0; j < sassRows.length; j++) {
                if (sassRows[j].src === null || sassRows[j].ptx === null) {
                    warn("SASS " + sassRows[j].addr + " 缺少源码行或 PTX 行");
                    break;
                }
            }
            if (steps.length) {
                var coverage = checkCoverage(steps, ptxRows, sassRows);
                if (coverage.missing.length || coverage.duplicated.length) {
                    warn("分步没有恰好覆盖每条 SASS：" + JSON.stringify(coverage));
                }
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

        var stepText = el("p", "cu-xref-step-text", "");
        stepText.setAttribute("aria-live", "polite");
        ui.appendChild(stepText);

        var ctx = {
            mode: mode,
            figure: figure,
            ui: ui,
            stepCount: stepCount,
            stepText: stepText,
            status: null,
            prev: prev,
            next: next,
            clear: clear,
            steps: steps,
            sourceRows: sourceRows,
            ptxRows: ptxRows,
            sassRows: sassRows,
            ptxByLine: ptxByLine,
            sassByAddr: sassByAddr,
            sassByPtx: sassByPtx,
            rows: [],
            sourcePane: null,
            ptxPane: null,
            sassPane: null,
            body: null,
            svg: null,
            step: 0,
            selected: { source: {}, ptx: {}, sass: {} },
            wide: null,
            pendingFrame: false
        };

        if (mode === "cu-ptx") {
            buildCuPtx(ctx, sourceRows, ptxRows, ranges);
        } else {
            buildPtxSass(ctx, ptxRows, sassRows);
        }

        var status = el("div", "cu-xref-status");
        status.setAttribute("role", "status");
        status.setAttribute("aria-live", "polite");
        status.textContent = "未选中任何行。";
        ui.appendChild(status);
        ctx.status = status;

        var caption = figure.querySelector("figcaption");
        var noscript = figure.querySelector("noscript");
        var anchor = noscript || caption;
        if (anchor && anchor.parentNode === figure) figure.insertBefore(ui, anchor);
        else figure.appendChild(ui);

        ctx.rows = figure.querySelectorAll(".cu-xref-row");
        contexts.push(ctx);

        for (var z = 0; z < ctx.rows.length; z++) {
            ctx.rows[z].addEventListener("click", function (event) {
                var row = event.currentTarget;
                select(ctx, row.getAttribute("data-kind"), row.getAttribute("data-key"));
            });
            ctx.rows[z].addEventListener("keydown", onRowKey);
        }
        prev.addEventListener("click", function () {
            if (ctx.step <= 1) return;
            updateStep(ctx, ctx.step - 1);
        });
        next.addEventListener("click", function () {
            updateStep(ctx, Math.min(ctx.step + 1, ctx.steps.length));
        });
        clear.addEventListener("click", function () {
            setStepZero(ctx);
        });

        if (mode === "ptx-sass") {
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
        }

        setStepZero(ctx);
        return ctx;
    }

    /* 图 4.2：源码在上，PTX 在下，各占整个正文宽度；不画连线。 */
    function buildCuPtx(ctx, sourceRows, ptxRows, ranges) {
        var sourceWrap = el("div", "cu-xref-block cu-xref-source-wrap");
        var sourceNote = sourceRows.length
            ? "第 " + sourceRows[0].line + "–" + sourceRows[sourceRows.length - 1].line + " 行"
            : "";
        sourceWrap.appendChild(makeHead("源码", sourceNote));

        var sourcePane = el("div", "cu-xref-pane cu-xref-source");
        var colorIndex = {};
        for (var i = 0; i < sourceRows.length; i++) {
            var srow = sourceRows[i];
            var sIndex = i < 4 ? i + 1 : 0;
            colorIndex[srow.line] = sIndex;
            var sLabel = "源码第 " + srow.line + " 行";
            var sNode = makeRow("source", srow.line, String(srow.line), srow.text, sLabel);
            sNode.setAttribute("data-line", String(srow.line));
            sNode.setAttribute("data-src", String(srow.line));
            sNode.className += " cu-xref-c" + sIndex;
            sourcePane.appendChild(sNode);
        }
        sourceWrap.appendChild(sourcePane);

        var ptxWrap = el("div", "cu-xref-block cu-xref-ptx-wrap");
        var ptxHead = makeHead("PTX", ptxRows.length + " 行 · 颜色来自 .loc");
        ptxWrap.appendChild(ptxHead);

        var ptxPane = el("div", "cu-xref-pane cu-xref-ptx");
        for (var q = 0; q < ptxRows.length; q++) {
            var prow = ptxRows[q];
            var cIndex = prow.src !== null && prow.src !== undefined
                ? (colorIndex[prow.src] || 0) : 0;
            var plabel = "PTX 第 " + prow.line + " 行";
            if (prow.src !== null && prow.src !== undefined) {
                plabel += "，对应源码第 " + prow.src + " 行";
            } else {
                plabel += "，没有源码行信息";
            }
            var pNode = makeRow("ptx", prow.line, String(prow.line), prow.text, plabel);
            pNode.setAttribute("data-ptx-line", String(prow.line));
            pNode.setAttribute("data-src",
                prow.src === null || prow.src === undefined ? "" : String(prow.src));
            pNode.className += " cu-xref-c" + cIndex;
            ptxPane.appendChild(pNode);
        }
        ptxWrap.appendChild(ptxPane);

        var stack = el("div", "cu-xref-stack");
        stack.appendChild(sourceWrap);
        stack.appendChild(ptxWrap);
        ctx.ui.appendChild(stack);

        ctx.sourcePane = sourcePane;
        ctx.ptxPane = ptxPane;
    }

    /* 图 4.3：PTX 函数体在左、SASS 在右，中间留出画连线的空间。 */
    function buildPtxSass(ctx, ptxRows, sassRows) {
        var body = el("div", "cu-xref-body");
        var svg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
        svg.setAttribute("class", "cu-xref-links");
        svg.setAttribute("aria-hidden", "true");
        body.appendChild(svg);

        var ptxSide = el("div", "cu-xref-side cu-xref-side-ptx");
        ptxSide.appendChild(makeHead("PTX", ptxRows.length + " 行 · 函数体"));
        var ptxPane = el("div", "cu-xref-pane cu-xref-ptx");
        for (var q = 0; q < ptxRows.length; q++) {
            var prow = ptxRows[q];
            var sassHere = ctx.sassByPtx[prow.line] || [];
            var plabel = "PTX 第 " + prow.line + " 行";
            if (prow.src !== null && prow.src !== undefined) {
                plabel += "，对应源码第 " + prow.src + " 行";
            }
            if (sassHere.length) plabel += "、SASS " + sassHere.join("、");
            var pNode = makeRow("ptx", prow.line, String(prow.line), prow.text, plabel);
            pNode.setAttribute("data-ptx-line", String(prow.line));
            ptxPane.appendChild(pNode);
        }
        ptxSide.appendChild(ptxPane);
        body.appendChild(ptxSide);

        var sassSide = el("div", "cu-xref-side cu-xref-side-sass");
        sassSide.appendChild(makeHead("SASS", sassRows.length + " 条"));
        var sassPane = el("div", "cu-xref-pane cu-xref-sass");
        for (var r = 0; r < sassRows.length; r++) {
            var arow = sassRows[r];
            var alabel = "SASS " + arow.addr;
            if (arow.src !== null && arow.src !== undefined) alabel += "，对应源码第 " + arow.src + " 行";
            if (arow.ptx !== null && arow.ptx !== undefined) alabel += "、PTX 第 " + arow.ptx + " 行";
            var aNode = makeRow("sass", arow.addr, arow.addr, arow.text, alabel);
            aNode.setAttribute("data-sass-addr", arow.addr);
            sassPane.appendChild(aNode);
        }
        sassSide.appendChild(sassPane);
        body.appendChild(sassSide);
        ctx.ui.appendChild(body);

        ctx.body = body;
        ctx.svg = svg;
        ctx.ptxPane = ptxPane;
        ctx.sassPane = sassPane;
    }

    function apply(ctx, sourceSet, ptxSet, sassSet, stepIndex) {
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
        ctx.figure.classList.toggle("is-selected", any);
        ctx.clear.disabled = !any && ctx.step === 0;
        ctx.prev.disabled = ctx.step <= 1;
        ctx.next.disabled = ctx.step >= ctx.steps.length || ctx.steps.length === 0;
        updateStatus(ctx);
        if (ctx.mode === "ptx-sass") drawLinks(ctx);
    }

    function updateStep(ctx, index) {
        if (!ctx.steps.length) return;
        if (index < 1) {
            setStepZero(ctx);
            return;
        }
        if (index > ctx.steps.length) index = ctx.steps.length;
        var step = ctx.steps[index - 1];
        ctx.stepCount.textContent = "第 " + index + " / " + ctx.steps.length + " 步";
        ctx.stepText.textContent = "";
        renderRichText(ctx.stepText, step.label);
        if (ctx.mode === "cu-ptx") {
            var sourceSelection = sourceStepSelection(step, ctx.ptxRows);
            apply(ctx, sourceSelection.source, sourceSelection.ptx, {}, index);
        } else {
            var selection = stepSelection(step, ctx.ptxRows, ctx.sassRows);
            apply(ctx, {}, selection.ptx, selection.sass, index);
        }
    }

    function select(ctx, kind, key) {
        if (ctx.mode === "cu-ptx") {
            if (kind === "source") {
                var line = parseInt(key, 10);
                var srcSet = {};
                var ptxSet = {};
                srcSet[line] = true;
                for (var i = 0; i < ctx.ptxRows.length; i++) {
                    if (ctx.ptxRows[i].src === line) ptxSet[ctx.ptxRows[i].line] = true;
                }
                ctx.stepCount.textContent = "第 0 / " + ctx.steps.length + " 步";
                ctx.stepText.textContent = defaultStepText(ctx);
                apply(ctx, srcSet, ptxSet, {}, 0);
                return;
            }
            if (kind === "ptx") {
                var ptxLine = parseInt(key, 10);
                var row = ctx.ptxByLine[ptxLine];
                var onePtx = {};
                var oneSrc = {};
                onePtx[ptxLine] = true;
                if (row && row.src !== null && row.src !== undefined) {
                    oneSrc[row.src] = true;
                    for (var j = 0; j < ctx.ptxRows.length; j++) {
                        if (ctx.ptxRows[j].src === row.src) onePtx[ctx.ptxRows[j].line] = true;
                    }
                }
                ctx.stepCount.textContent = "第 0 / " + ctx.steps.length + " 步";
                ctx.stepText.textContent = defaultStepText(ctx);
                apply(ctx, oneSrc, onePtx, {}, 0);
                return;
            }
            return;
        }

        if (kind === "ptx") {
            var lineNo = parseInt(key, 10);
            var ptxRow = ctx.ptxByLine[lineNo];
            var ptxOne = {};
            var sassOne = {};
            ptxOne[lineNo] = true;
            for (var k = 0; k < ctx.sassRows.length; k++) {
                if (ctx.sassRows[k].ptx === lineNo) sassOne[ctx.sassRows[k].addr] = true;
            }
            ctx.stepCount.textContent = "第 0 / " + ctx.steps.length + " 步";
            ctx.stepText.textContent = defaultStepText(ctx);
            apply(ctx, {}, ptxOne, sassOne, 0);
            return;
        }

        var sassRow = ctx.sassByAddr[key];
        if (!sassRow) return;
        var sassKey = {};
        var ptxKey = {};
        sassKey[sassRow.addr] = true;
        ptxKey[sassRow.ptx] = true;
        ctx.stepCount.textContent = "第 0 / " + ctx.steps.length + " 步";
        ctx.stepText.textContent = defaultStepText(ctx);
        apply(ctx, {}, ptxKey, sassKey, 0);
    }

    function onRowKey(event) {
        var row = event.currentTarget;
        if (event.key === "Enter" || event.key === " " || event.key === "Spacebar") {
            event.preventDefault();
            event.stopPropagation();
            var ctx = contextFor(row);
            if (ctx) select(ctx, row.getAttribute("data-kind"), row.getAttribute("data-key"));
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

    function contextFor(node) {
        for (var i = 0; i < contexts.length; i++) {
            if (contexts[i].figure.contains(node)) return contexts[i];
        }
        return null;
    }

    function sortNumeric(keys) {
        return keys.sort(function (a, b) { return parseInt(a, 10) - parseInt(b, 10); });
    }

    function updateStatus(ctx) {
        var src = sortNumeric(Object.keys(ctx.selected.source));
        var ptx = sortNumeric(Object.keys(ctx.selected.ptx));
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
        if (ctx.mode !== "ptx-sass" || !ctx.svg) return;
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
        if (ctx.mode !== "ptx-sass" || ctx.pendingFrame) return;
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
        var roots = document.querySelectorAll(".cu-xref");
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
            parsePtxBody: parsePtxBody,
            parseSassMap: parseSassMap,
            parseSass: parseSass,
            isUnconditionalExit: isUnconditionalExit,
            parseSteps: parseSteps,
            sourceStepSelection: sourceStepSelection,
            stepSelection: stepSelection,
            checkCoverage: checkCoverage
        };
    }
})();
