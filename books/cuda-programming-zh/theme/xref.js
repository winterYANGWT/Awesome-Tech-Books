/* 第 4 章 图 4.2 / 图 4.3：源码 → PTX → SASS 对照。
 *
 * 页面里只放工具原样输出（text/plain），这里按 <figure data-mode> 分两种模式：
 *   - cu-ptx：源码在左、PTX 在右，按 .loc 着色并画对应连线；
 *   - ptx-sass：PTX 函数体和 SASS 左右排，中间画对应连线。
 * 外层 figure.cu-xref-tabs 把两种模式做成标签页。
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

    /* 一行属于哪一步：steps 里第 field 个字段（cu / ptx）包含 value 的步号，
       从 1 开始；找不到返回 0。 */
    function stepIndexFor(steps, field, value) {
        var target = parseInt(value, 10);
        if (!isFinite(target) || target < 1) return 0;
        for (var i = 0; i < steps.length; i++) {
            var list = steps[i][field] || [];
            for (var j = 0; j < list.length; j++) {
                if (list[j] === target) return i + 1;
            }
        }
        return 0;
    }

    function sourceStepIndex(steps, line) {
        return stepIndexFor(steps, "cu", line);
    }

    function ptxStepIndex(steps, line) {
        return stepIndexFor(steps, "ptx", line);
    }

    function rowByLine(rows, line) {
        var target = parseInt(line, 10);
        for (var i = 0; i < rows.length; i++) {
            if (rows[i].line === target) return rows[i];
        }
        return null;
    }

    function sassRowByAddr(rows, addr) {
        var key = String(addr === null || addr === undefined ? "" : addr).toLowerCase();
        for (var i = 0; i < rows.length; i++) {
            if (String(rows[i].addr).toLowerCase() === key) return rows[i];
        }
        return null;
    }

    /* 图 4.2 左栏：源码行 → 包含该行的那一步。 */
    function cuPtxStepForSource(steps, line) {
        return sourceStepIndex(steps, line);
    }

    /* 图 4.2 左栏：PTX 行 → 它的 .loc 源码行 → 那一步。 */
    function cuPtxStepForPtx(steps, ptxRows, line) {
        var row = rowByLine(ptxRows, line);
        if (!row || row.src === null || row.src === undefined) return 0;
        return sourceStepIndex(steps, row.src);
    }

    /* 图 4.2 右栏：PTX 行 → 包含它的那一步。 */
    function ptxSassStepForPtx(steps, line) {
        return ptxStepIndex(steps, line);
    }

    /* 图 4.2 右栏：SASS 行 → 它的 PTX 行 → 那一步。 */
    function ptxSassStepForSass(steps, sassRows, addr) {
        var row = sassRowByAddr(sassRows, addr);
        if (!row || row.ptx === null || row.ptx === undefined) return 0;
        return ptxStepIndex(steps, row.ptx);
    }

    function selectedHas(collection, key) {
        if (!collection) return false;
        if (Array.isArray(collection)) {
            for (var i = 0; i < collection.length; i++) {
                if (String(collection[i]) === String(key)) return true;
            }
            return false;
        }
        return collection[key] === true || collection[String(key)] === true;
    }

    /* 图 4.2 左栏当前要画的连线：[源码行, PTX 行] 对。每条亮起的 PTX
       都连到它 .loc 所属的源码行；选中状态可以是对象集合，也可以是数组。 */
    function cuPtxLinkPairs(selectedSource, selectedPtx, ptxRows) {
        var lines = [];
        var seen = {};
        var i;
        if (Array.isArray(selectedSource)) {
            for (i = 0; i < selectedSource.length; i++) {
                var value = parseInt(selectedSource[i], 10);
                if (isFinite(value) && value > 0 && !seen[value]) {
                    seen[value] = true;
                    lines.push(value);
                }
            }
        } else if (selectedSource) {
            for (var key in selectedSource) {
                if (!Object.prototype.hasOwnProperty.call(selectedSource, key)) continue;
                if (!selectedSource[key]) continue;
                var parsed = parseInt(key, 10);
                if (isFinite(parsed) && parsed > 0 && !seen[parsed]) {
                    seen[parsed] = true;
                    lines.push(parsed);
                }
            }
        }
        lines.sort(function (a, b) { return a - b; });

        var out = [];
        for (i = 0; i < lines.length; i++) {
            for (var j = 0; j < ptxRows.length; j++) {
                var row = ptxRows[j];
                if (row.src === lines[i] && selectedHas(selectedPtx, row.line)) {
                    out.push([lines[i], row.line]);
                }
            }
        }
        return out;
    }

    /* ------------------------------------------------------------------ *
     * DOM
     * ------------------------------------------------------------------ */

    var contexts = [];
    var tabSequence = 0;
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

    /* 图下面那段分析的第一个 <ol>：第 k 个 li 对应第 k 步。
       条数和步数不等时只警告，图本身照常工作。 */
    function setupNotes(ctx) {
        var scope = ctx.figure.parentNode;
        var notesEl = null;
        if (scope && scope.classList && scope.classList.contains("cu-xref-tabs")) {
            notesEl = scope.querySelector(
                '.cu-xref-notes[data-for="' + ctx.mode + '"]');
        } else {
            notesEl = ctx.figure.querySelector(".cu-xref-notes");
        }
        if (!notesEl) return null;

        var list = notesEl.querySelector("ol");
        var items = [];
        if (list) {
            for (var i = 0; i < list.children.length; i++) {
                if (String(list.children[i].tagName).toLowerCase() === "li") {
                    items.push(list.children[i]);
                }
            }
        }
        if (!ctx.steps.length || items.length !== ctx.steps.length) {
            warn("分析里的条目数（" + items.length + "）和步数（" +
                ctx.steps.length + "）不一致，跳过分析和步骤的联动");
            return null;
        }

        for (var k = 0; k < items.length; k++) {
            (function (index) {
                var item = items[index];
                item.setAttribute("tabindex", "0");
                item.addEventListener("click", function () {
                    if (selectionInside(item)) return;
                    goToNote(ctx, index + 1);
                });
                item.addEventListener("keydown", function (event) {
                    if (event.key === "Enter" || event.key === " " ||
                        event.key === "Spacebar") {
                        event.preventDefault();
                        event.stopPropagation();
                        goToNote(ctx, index + 1);
                        return;
                    }
                    if (event.key === "ArrowLeft" || event.key === "ArrowRight") {
                        event.stopPropagation();
                    }
                });
            })(k);
        }
        return { el: notesEl, items: items };
    }

    function selectionInside(node) {
        if (typeof window === "undefined" || !window.getSelection) return false;
        var selection = window.getSelection();
        if (!selection || selection.isCollapsed || !selection.rangeCount) return false;
        try {
            return node.contains(selection.getRangeAt(0).commonAncestorContainer);
        } catch (err) {
            return false;
        }
    }

    function goToNote(ctx, index) {
        updateStep(ctx, index);
        var panel = ctx.figure;
        if (!panel || panel.getBoundingClientRect().top >= 0) return;
        var reduce = false;
        if (typeof window.matchMedia === "function") {
            reduce = window.matchMedia("(prefers-reduced-motion: reduce)").matches;
        }
        try {
            panel.scrollIntoView({
                block: "nearest",
                behavior: reduce ? "auto" : "smooth"
            });
        } catch (err) {
            panel.scrollIntoView(false);
        }
    }

    function setCurrentStep(ctx, index) {
        ctx.currentStep = index;
        if (!ctx.notes) return;
        var items = ctx.notes.items;
        for (var i = 0; i < items.length; i++) {
            items[i].classList.remove("is-current");
        }
        if (index < 1 || index > items.length) return;
        var item = items[index - 1];
        item.classList.add("is-current");
        var color = 1;
        if (ctx.mode === "cu-ptx") {
            var step = ctx.steps[index - 1];
            color = step && step.cu.length
                ? (ctx.sourceColor[step.cu[0]] || 0) : 0;
        }
        item.style.setProperty("--cu-note",
            color ? "var(--cu-s" + color + ")" : "var(--cu-xref-neutral)");
    }

    /* 点某一行时，那一行属于哪一步；用于分析里 li 的高亮。 */
    function stepForRow(ctx, kind, key) {
        if (ctx.mode === "cu-ptx") {
            if (kind === "source") return cuPtxStepForSource(ctx.steps, key);
            if (kind === "ptx") return cuPtxStepForPtx(ctx.steps, ctx.ptxRows, key);
            return 0;
        }
        if (kind === "ptx") return ptxSassStepForPtx(ctx.steps, key);
        if (kind === "sass") return ptxSassStepForSass(ctx.steps, ctx.sassRows, key);
        return 0;
    }

    function setStepZero(ctx) {
        ctx.step = 0;
        if (ctx.steps.length) {
            ctx.stepCount.textContent = "第 0 / " + ctx.steps.length + " 步";
        }
        ctx.stepText.textContent = defaultStepText(ctx);
        setCurrentStep(ctx, 0);
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
            currentStep: 0,
            notes: null,
            visible: true,
            sourceColor: {},
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

        if (typeof window.matchMedia === "function") {
            ctx.wide = window.matchMedia(WIDE);
            var onMode = function () { scheduleRefresh(ctx); };
            if (ctx.wide.addEventListener) ctx.wide.addEventListener("change", onMode);
            else if (ctx.wide.addListener) ctx.wide.addListener(onMode);
        }

        window.addEventListener("resize", function () {
            scheduleRefresh(ctx);
        });
        if (typeof ResizeObserver === "function") {
            var observer = new ResizeObserver(function () { scheduleRefresh(ctx); });
            if (ctx.body) observer.observe(ctx.body);
            if (ctx.sourcePane) observer.observe(ctx.sourcePane);
            if (ctx.ptxPane) observer.observe(ctx.ptxPane);
        }
        if (document.fonts && document.fonts.ready && document.fonts.ready.then) {
            document.fonts.ready.then(function () { scheduleRefresh(ctx); });
        }

        ctx.notes = setupNotes(ctx);
        setStepZero(ctx);
        return ctx;
    }

    /* 图 4.2 左栏：源码在左、PTX 在右，中间画对应连线。源码行自动折行，
       PTX 保持单行、放不下时栏内横向滚动。 */
    function buildCuPtx(ctx, sourceRows, ptxRows, ranges) {
        var body = el("div", "cu-xref-body cu-xref-body-cu-ptx");
        var svg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
        svg.setAttribute("class", "cu-xref-links");
        svg.setAttribute("aria-hidden", "true");
        body.appendChild(svg);

        var sourceSide = el("div", "cu-xref-side cu-xref-side-source");
        var sourceNote = sourceRows.length
            ? "第 " + sourceRows[0].line + "–" + sourceRows[sourceRows.length - 1].line + " 行"
            : "";
        sourceSide.appendChild(makeHead("源码", sourceNote));

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
        sourceSide.appendChild(sourcePane);
        body.appendChild(sourceSide);

        var ptxSide = el("div", "cu-xref-side cu-xref-side-ptx");
        ptxSide.appendChild(makeHead("PTX", ptxRows.length + " 行 · 颜色来自 .loc"));
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
        ptxSide.appendChild(ptxPane);
        body.appendChild(ptxSide);

        ctx.ui.appendChild(body);
        ctx.body = body;
        ctx.svg = svg;
        ctx.sourcePane = sourcePane;
        ctx.ptxPane = ptxPane;
        ctx.sourceColor = colorIndex;
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
        scheduleRefresh(ctx);
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
        setCurrentStep(ctx, index);
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
                setCurrentStep(ctx, stepForRow(ctx, kind, key));
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
                setCurrentStep(ctx, stepForRow(ctx, kind, key));
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
            setCurrentStep(ctx, stepForRow(ctx, kind, key));
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
        setCurrentStep(ctx, stepForRow(ctx, kind, key));
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

    function findRowNode(ctx, kind, key) {
        for (var i = 0; i < ctx.rows.length; i++) {
            if (ctx.rows[i].getAttribute("data-kind") === kind &&
                ctx.rows[i].getAttribute("data-key") === String(key)) {
                return ctx.rows[i];
            }
        }
        return null;
    }

    function isNarrowViewport() {
        if (typeof window === "undefined") return false;
        if (typeof window.matchMedia === "function") {
            return window.matchMedia("(max-width: 699px)").matches;
        }
        return typeof window.innerWidth === "number" && window.innerWidth < 700;
    }

    function makeLink(svg, x1, y1, x2, y2, colorClass) {
        var bend = Math.max(12, (x2 - x1) * 0.42);
        var path = document.createElementNS("http://www.w3.org/2000/svg", "path");
        path.setAttribute("class", colorClass
            ? "cu-xref-link " + colorClass : "cu-xref-link");
        path.setAttribute("d",
            "M" + x1.toFixed(1) + "," + y1.toFixed(1) +
            " C" + (x1 + bend).toFixed(1) + "," + y1.toFixed(1) +
            " " + (x2 - bend).toFixed(1) + "," + y2.toFixed(1) +
            " " + x2.toFixed(1) + "," + y2.toFixed(1));
        path.setAttribute("pathLength", "1");
        svg.appendChild(path);
    }

    /* 两条栏之间画当前选中项的连线。cu-ptx 用源码行的颜色；
       ptx-sass 固定用系列色 1。窄屏不画。 */
    function drawLinks(ctx) {
        if (!ctx.svg || !ctx.body) return;
        var svg = ctx.svg;
        while (svg.firstChild) svg.removeChild(svg.firstChild);
        if (ctx.visible === false || ctx.figure.hidden) return;
        if (ctx.wide && !ctx.wide.matches) return;
        if (typeof window === "undefined") return;

        var bodyRect = ctx.body.getBoundingClientRect();
        var width = Math.max(0, Math.round(bodyRect.width));
        var height = Math.max(0, Math.round(bodyRect.height));
        if (!width || !height) return;

        var leftRect;
        var rightRect;
        var pairs = [];
        var i;
        if (ctx.mode === "cu-ptx") {
            leftRect = ctx.sourcePane.getBoundingClientRect();
            rightRect = ctx.ptxPane.getBoundingClientRect();
            var linkPairs = cuPtxLinkPairs(ctx.selected.source, ctx.selected.ptx,
                ctx.ptxRows);
            for (i = 0; i < linkPairs.length; i++) {
                var sourceLine = linkPairs[i][0];
                var sourceNode = findRowNode(ctx, "source", sourceLine);
                var targetNode = findRowNode(ctx, "ptx", linkPairs[i][1]);
                if (!sourceNode || !targetNode) continue;
                pairs.push({
                    from: sourceNode,
                    to: targetNode,
                    color: "cu-xref-c" + (ctx.sourceColor[sourceLine] || 0)
                });
            }
        } else {
            leftRect = ctx.ptxPane.getBoundingClientRect();
            rightRect = ctx.sassPane.getBoundingClientRect();
            var sassKeys = Object.keys(ctx.selected.sass);
            for (i = 0; i < sassKeys.length; i++) {
                var sassRow = ctx.sassByAddr[sassKeys[i]];
                if (!sassRow) continue;
                if (!ctx.selected.ptx[sassRow.ptx]) continue;
                var ptxNode = findRowNode(ctx, "ptx", sassRow.ptx);
                var sassNode = findRowNode(ctx, "sass", sassRow.addr);
                if (!ptxNode || !sassNode) continue;
                pairs.push({ from: ptxNode, to: sassNode, color: "" });
            }
        }

        svg.setAttribute("width", String(width));
        svg.setAttribute("height", String(height));
        svg.setAttribute("viewBox", "0 0 " + width + " " + height);

        var x1 = leftRect.right - bodyRect.left + 4;
        var x2 = rightRect.left - bodyRect.left - 4;
        if (x2 <= x1) return;
        for (i = 0; i < pairs.length; i++) {
            var r1 = pairs[i].from.getBoundingClientRect();
            var r2 = pairs[i].to.getBoundingClientRect();
            makeLink(svg, x1, r1.top + r1.height / 2 - bodyRect.top,
                x2, r2.top + r2.height / 2 - bodyRect.top, pairs[i].color);
        }
    }

    function refresh(ctx) {
        if (!ctx || ctx.visible === false || ctx.figure.hidden) return;
        drawLinks(ctx);
    }

    function scheduleRefresh(ctx) {
        if (!ctx || ctx.pendingFrame) return;
        ctx.pendingFrame = true;
        var raf = typeof window.requestAnimationFrame === "function"
            ? window.requestAnimationFrame.bind(window)
            : function (fn) { return setTimeout(fn, 16); };
        raf(function () {
            ctx.pendingFrame = false;
            refresh(ctx);
        });
    }

    /* 标签页：一次只显示一个面板和 data-for 等于它 data-mode 的说明。 */
    function setupTabs(container) {
        var panels = [];
        for (var c = 0; c < container.children.length; c++) {
            var child = container.children[c];
            if (child.classList && child.classList.contains("cu-xref") &&
                child.hasAttribute("data-tab")) {
                panels.push(child);
            }
        }
        if (!panels.length) return;

        var panelCtx = [];
        for (var p = 0; p < panels.length; p++) panelCtx.push(setup(panels[p]));

        var base = container.id || ("cu-xref-tabs-" + (++tabSequence));
        container.id = base;
        container.classList.add("cu-xref-tabs-ready");

        var bar = el("div", "cu-xref-tabs-bar");
        bar.setAttribute("role", "tablist");
        bar.setAttribute("aria-label", "图 4.2 的两次翻译");
        var buttons = [];
        var notes = [];
        var noteNodes = container.querySelectorAll(".cu-xref-notes");
        for (var j = 0; j < noteNodes.length; j++) notes.push(noteNodes[j]);

        for (var t = 0; t < panels.length; t++) {
            (function (index) {
                var panel = panels[index];
                panel.id = base + "-panel-" + index;
                panel.setAttribute("role", "tabpanel");
                var button = el("button", "cu-xref-tab", panel.getAttribute("data-tab"));
                button.type = "button";
                button.id = base + "-tab-" + index;
                button.setAttribute("role", "tab");
                button.setAttribute("aria-controls", panel.id);
                panel.setAttribute("aria-labelledby", button.id);
                button.addEventListener("click", function () {
                    activate(index, false, true);
                });
                bar.appendChild(button);
                buttons.push(button);
            })(t);
        }
        container.insertBefore(bar, container.firstChild);

        var current = 0;

        function fade(node, animate) {
            if (!node || !animate) return;
            node.classList.remove("cu-xref-tab-enter");
            void node.offsetWidth;
            node.classList.add("cu-xref-tab-enter");
            setTimeout(function () {
                node.classList.remove("cu-xref-tab-enter");
            }, 220);
        }

        function activate(index, focus, animate) {
            if (index < 0) index = 0;
            if (index >= panels.length) index = panels.length - 1;
            current = index;
            var mode = panels[index].getAttribute("data-mode");
            for (var k = 0; k < panels.length; k++) {
                var on = k === index;
                panels[k].hidden = !on;
                buttons[k].setAttribute("aria-selected", on ? "true" : "false");
                buttons[k].tabIndex = on ? 0 : -1;
                if (!on) panels[k].classList.remove("cu-xref-tab-enter");
                if (panelCtx[k]) panelCtx[k].visible = on;
            }
            for (var n = 0; n < notes.length; n++) {
                var show = (notes[n].getAttribute("data-for") || "") === mode;
                notes[n].hidden = !show;
                if (!show) notes[n].classList.remove("cu-xref-tab-enter");
            }
            if (animate) {
                fade(panels[index], true);
                for (var m = 0; m < notes.length; m++) {
                    if (!notes[m].hidden) fade(notes[m], true);
                }
            }
            for (var q = 0; q < panelCtx.length; q++) {
                if (panelCtx[q] && panelCtx[q].visible) scheduleRefresh(panelCtx[q]);
            }
            if (focus) buttons[index].focus();
        }

        bar.addEventListener("keydown", function (event) {
            var key = event.key;
            if (key !== "ArrowLeft" && key !== "ArrowRight" &&
                key !== "Home" && key !== "End") return;
            // mdBook 在 document 上用左右键翻章；标签页里必须拦住。
            event.preventDefault();
            event.stopPropagation();
            var to = current;
            if (key === "ArrowLeft") to = (current - 1 + panels.length) % panels.length;
            else if (key === "ArrowRight") to = (current + 1) % panels.length;
            else if (key === "Home") to = 0;
            else if (key === "End") to = panels.length - 1;
            activate(to, true, true);
        });

        activate(0, false, false);
    }

    /* 桌面端让整张图比正文列宽：min(1040, 内容区可用宽度)，相对正文列居中。
       用 ResizeObserver 盯着内容区和 <main>，侧栏、窗口、阅读器调宽度都会触发。 */
    function fitTabsWidth(container) {
        var content = document.getElementById("mdbook-content");
        var main = content ? content.querySelector("main") : null;
        if (!content || !main) return;
        var mainRect = main.getBoundingClientRect();
        var mainWidth = mainRect.width;
        if (!mainWidth) return;
        var style = window.getComputedStyle(content);
        var padLeft = parseFloat(style.paddingLeft) || 0;
        var padRight = parseFloat(style.paddingRight) || 0;
        var available = Math.max(0, content.clientWidth - padLeft - padRight);
        var target = Math.floor(Math.min(1040, available));
        var narrow = isNarrowViewport();
        container.style.setProperty("--cu-xref-col-width",
            mainWidth.toFixed(2) + "px");
        if (!narrow && target > mainWidth + 0.5) {
            container.style.width = target + "px";
            container.style.maxWidth = "none";
            container.style.marginLeft =
                ((mainWidth - target) / 2).toFixed(2) + "px";
            container.classList.add("is-wide");
        } else {
            container.style.width = "";
            container.style.maxWidth = "";
            container.style.marginLeft = "";
            container.classList.remove("is-wide");
        }
    }

    function initTabsWidth(container) {
        var content = document.getElementById("mdbook-content");
        var main = content ? content.querySelector("main") : null;
        if (!content || !main) return;
        fitTabsWidth(container);
        // 直接量、直接改：ResizeObserver 回调在绘制前执行，侧栏动画过程中
        // 也不会出现某一帧图比可用宽度还宽、把页面撑出横向滚动条。
        var schedule = function () { fitTabsWidth(container); };
        if (typeof ResizeObserver === "function") {
            var observer = new ResizeObserver(schedule);
            observer.observe(content);
            observer.observe(main);
            container._cuXrefWidthObserver = observer;
        }
        window.addEventListener("resize", schedule);
        if (document.fonts && document.fonts.ready && document.fonts.ready.then) {
            document.fonts.ready.then(schedule);
        }
    }

    function insideTabs(node) {
        var parent = node.parentNode;
        while (parent && parent !== document) {
            if (parent.classList && parent.classList.contains("cu-xref-tabs")) {
                return true;
            }
            parent = parent.parentNode;
        }
        return false;
    }

    function init() {
        var containers = document.querySelectorAll("figure.cu-xref-tabs");
        for (var i = 0; i < containers.length; i++) {
            try {
                setupTabs(containers[i]);
                initTabsWidth(containers[i]);
            } catch (err) {
                warn("标签页初始化失败", err);
            }
        }

        var roots = document.querySelectorAll(".cu-xref");
        if (!roots.length) return;
        Array.prototype.forEach.call(roots, function (root) {
            if (insideTabs(root)) return;
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
            checkCoverage: checkCoverage,
            stepIndexFor: stepIndexFor,
            sourceStepIndex: sourceStepIndex,
            ptxStepIndex: ptxStepIndex,
            rowByLine: rowByLine,
            sassRowByAddr: sassRowByAddr,
            cuPtxStepForSource: cuPtxStepForSource,
            cuPtxStepForPtx: cuPtxStepForPtx,
            ptxSassStepForPtx: ptxSassStepForPtx,
            ptxSassStepForSass: ptxSassStepForSass,
            cuPtxLinkPairs: cuPtxLinkPairs
        };
    }
})();
