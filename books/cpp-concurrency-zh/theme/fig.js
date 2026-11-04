/* 图的纯函数渲染器 + 浏览器播放器。
 *
 * 纯函数部分（textWidth / layout / renderSVG）不碰 DOM，可以在 Node 里
 * require 后测试。浏览器部分只在 document 存在时运行：读 .ccfig-spec，
 * 把 SVG 画进 .ccfig-stage，并加播放控件、键盘、自动播放和图表悬停。
 */
(function () {
    "use strict";

    /* ------------------------------------------------------------------ *
     * 基础工具
     * ------------------------------------------------------------------ */

    function clamp(value, lo, hi) {
        return value < lo ? lo : (value > hi ? hi : value);
    }

    function esc(text) {
        return String(text).replace(/[&<>"']/g, function (ch) {
            return { "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[ch];
        });
    }

    function attr(text) {
        return esc(text);
    }

    function isCJK(code) {
        return (code >= 0x2e80 && code <= 0x9fff) ||
            (code >= 0x3000 && code <= 0x303f) ||
            (code >= 0x3400 && code <= 0x4dbf) ||
            (code >= 0xf900 && code <= 0xfaff) ||
            (code >= 0xfe30 && code <= 0xfe4f) ||
            (code >= 0xff00 && code <= 0xffef) ||
            (code >= 0x20000 && code <= 0x2fa1f);
    }

    /* 一个字符的估算宽度：汉字/全角 1.0，ASCII 0.6（等宽 0.62），其他 0.6。 */
    function textWidth(text, fontSize, mono) {
        var total = 0;
        var s = String(text);
        for (var i = 0; i < s.length; i++) {
            var code = s.codePointAt(i);
            if (code > 0xffff) i += 1;
            if (isCJK(code)) total += 1.0 * fontSize;
            else if (code < 128) total += (mono ? 0.62 : 0.6) * fontSize;
            else total += 0.6 * fontSize;
        }
        return total;
    }

    function splitLines(text) {
        return String(text === undefined || text === null ? "" : text).split("\n");
    }

    function maxLineWidth(text, fontSize, mono) {
        var lines = splitLines(text);
        var best = 0;
        for (var i = 0; i < lines.length; i++) {
            best = Math.max(best, textWidth(lines[i], fontSize, mono));
        }
        return best;
    }

    function lineCount(text) {
        return splitLines(text).length;
    }

    function inlineSVG(text) {
        var s = String(text);
        var out = "";
        var i = 0;
        while (i < s.length) {
            var j = s.indexOf("`", i);
            if (j < 0) {
                out += esc(s.slice(i));
                break;
            }
            if (j > i) out += esc(s.slice(i, j));
            var k = s.indexOf("`", j + 1);
            if (k < 0) {
                out += esc(s.slice(j));
                break;
            }
            out += '<tspan class="ccfig-code">' + esc(s.slice(j + 1, k)) + "</tspan>";
            i = k + 1;
        }
        return out;
    }

    function textSVG(x, y, text, fontSize, cls, anchor, mono) {
        var lines = splitLines(text);
        var lineHeight = fontSize * 1.22;
        var y0 = y - (lines.length - 1) * lineHeight / 2;
        var out = '<text class="' + (cls || "ccfig-label") + '" x="' + r2(x) + '" y="' + r2(y0) +
            '" font-size="' + r2(fontSize) + '" text-anchor="' + (anchor || "middle") +
            '" dominant-baseline="central"' + (mono ? ' font-family="var(--ccfig-mono)"' : "") + ">";
        for (var i = 0; i < lines.length; i++) {
            out += '<tspan x="' + r2(x) + '" y="' + r2(y0 + i * lineHeight) + '">' +
                inlineSVG(lines[i]) + "</tspan>";
        }
        return out + "</text>";
    }

    function r2(value) {
        return Math.round(value * 100) / 100;
    }

    function colorClass(index) {
        return "ccfig-c" + ((index % 8) + 1);
    }

    function toneClass(tone) {
        return tone ? " ccfig-tone-" + tone : "";
    }

    function addText(texts, text, x, y, fontSize, maxWidth, mono, anchor, meta) {
        var lines = splitLines(text);
        var lineHeight = fontSize * 1.22;
        var blockH = lines.length * lineHeight;
        for (var i = 0; i < lines.length; i++) {
            var item = {
                text: lines[i],
                x: x,
                y: y + (i - (lines.length - 1) / 2) * lineHeight,
                fontSize: fontSize,
                maxWidth: maxWidth,
                mono: !!mono,
                anchor: anchor || "middle"
            };
            if (meta) {
                item.blockId = meta.blockId;
                item.blockH = blockH;
                item.blockTop = y - blockH / 2;
                item.blockBottom = y + blockH / 2;
                if (meta.box) {
                    item.boxX = meta.box.x;
                    item.boxY = meta.box.y;
                    item.boxW = meta.box.w;
                    item.boxH = meta.box.h;
                }
            }
            texts.push(item);
        }
    }

    function svgOpen(width, height, spec, extraClass) {
        return '<svg xmlns="http://www.w3.org/2000/svg" class="ccfig-svg' +
            (extraClass ? " " + extraClass : "") + '" viewBox="0 0 ' + r2(width) + " " + r2(height) +
            '" width="' + r2(width) + '" height="' + r2(height) +
            '" style="--ccfig-w:' + r2(width) + 'px;--ccfig-min:' + r2(width * 0.7) +
            'px" role="img" aria-label="' +
            attr(spec.alt || "") + '">';
    }

    function rectSVG(x, y, w, h, cls, extra) {
        return '<rect class="' + cls + '" x="' + r2(x) + '" y="' + r2(y) + '" width="' +
            r2(w) + '" height="' + r2(h) + '"' + (extra || "") + "/>";
    }

    function arrowHead(x, y, dx, dy, cls) {
        var len = Math.sqrt(dx * dx + dy * dy) || 1;
        var ux = dx / len;
        var uy = dy / len;
        var size = 7;
        var bx = x - ux * size;
        var by = y - uy * size;
        var px = -uy * size * 0.55;
        var py = ux * size * 0.55;
        return '<polygon class="' + cls + '" points="' + r2(x) + "," + r2(y) + " " +
            r2(bx + px) + "," + r2(by + py) + " " + r2(bx - px) + "," + r2(by - py) + '"/>';
    }

    function parseRange(value, limit, name) {
        /* 返回 [start, end]（含端点）；用于 grid 的 r/c。 */
        if (value === "*") return [0, limit - 1];
        if (typeof value === "number") {
            var n = Math.trunc(value);
            return [n, n];
        }
        if (Array.isArray(value)) {
            var lo = limit, hi = -1;
            for (var i = 0; i < value.length; i++) {
                var one = parseRange(value[i], limit, name)[0];
                lo = Math.min(lo, one);
                hi = Math.max(hi, one);
            }
            return [lo, hi];
        }
        var m = /^\s*(-?\d+)\s*-\s*(-?\d+)\s*$/.exec(String(value));
        if (m) return [parseInt(m[1], 10), parseInt(m[2], 10)];
        throw new Error("bad " + name + " range: " + value);
    }

    /* ------------------------------------------------------------------ *
     * seq
     * ------------------------------------------------------------------ */

    function seqOps(step) {
        if (Array.isArray(step.ops)) return step.ops;
        if (step.from !== undefined || step.to !== undefined || step.span !== undefined ||
            step.at !== undefined) return [step];
        return [];
    }

    function layoutSeq(spec) {
        var lanes = spec.lanes || [];
        var steps = spec.steps || [];
        var margin = 14;
        var headerH = 42;
        var baseRow = 36;
        var lineH = 16;
        var font = 13;
        var labelFont = 12;
        var texts = [];
        var widthScale = 1;

        var widths = [];
        var i;
        for (i = 0; i < lanes.length; i++) {
            widths.push(Math.max(110, maxLineWidth(lanes[i].label || "", labelFont, false) + 24));
        }

        function opTextWidth(op) {
            if (op.text === undefined) return 0;
            return maxLineWidth(op.text, font, false);
        }

        function ensureArrow(i, j, text) {
            var lo = Math.min(i, j);
            var hi = Math.max(i, j);
            var distance = 0;
            for (var k = lo; k < hi; k++) distance += widths[k];
            var need = opTextWidth({ text: text }) + 18;
            if (distance < need) {
                var add = (need - distance) / (hi - lo);
                for (var q = lo; q < hi; q++) widths[q] += add;
            }
        }

        function ensureSpan(idxs, text) {
            var lo = Math.min.apply(null, idxs);
            var hi = Math.max.apply(null, idxs);
            var total = 0;
            for (var k = lo; k <= hi; k++) total += widths[k];
            var need = opTextWidth({ text: text }) + 24;
            if (total < need) {
                var add = (need - total) / (hi - lo + 1);
                for (var q = lo; q <= hi; q++) widths[q] += add;
            }
        }

        for (var pass = 0; pass < 3; pass++) {
            for (var si = 0; si < steps.length; si++) {
                var ops = seqOps(steps[si]);
                for (var oi = 0; oi < ops.length; oi++) {
                    var op = ops[oi];
                    if (op.at !== undefined) {
                        var atIndex = laneIndex(lanes, op.at);
                        widths[atIndex] = Math.max(widths[atIndex], opTextWidth(op) + 20);
                    } else if (op.span !== undefined) {
                        var spanIdxs = op.span.map(function (id) { return laneIndex(lanes, id); });
                        ensureSpan(spanIdxs, op.text || "");
                    } else if (op.from !== undefined && op.to !== undefined) {
                        ensureArrow(laneIndex(lanes, op.from), laneIndex(lanes, op.to), op.text || "");
                    }
                }
            }
        }

        var hasRowLabel = false;
        var labelColW = 0;
        for (var lri = 0; lri < steps.length; lri++) {
            if (steps[lri].label) {
                hasRowLabel = true;
                labelColW = Math.max(labelColW, maxLineWidth(steps[lri].label, 12, false) + 18);
            }
        }
        if (hasRowLabel) labelColW = Math.max(labelColW, 56);
        var rawWidth = widths.reduce(function (a, b) { return a + b; }, 0) +
            margin * 2 + labelColW;
        if (rawWidth > 760) widthScale = 760 / rawWidth;
        if (widthScale !== 1) {
            for (i = 0; i < widths.length; i++) widths[i] *= widthScale;
            margin *= widthScale;
            labelColW *= widthScale;
            headerH *= widthScale;
            baseRow *= widthScale;
            lineH *= widthScale;
            font *= widthScale;
            labelFont *= widthScale;
        }

        var left = [];
        var centers = [];
        var x = margin + labelColW;
        for (i = 0; i < lanes.length; i++) {
            left.push(x);
            centers.push(x + widths[i] / 2);
            x += widths[i];
        }
        var width = x + margin;

        /* 泳道状态：new 从某一步开始，done 到某一步结束。 */
        var startStep = {};
        var statusBefore = {};
        var statusAfter = {};
        var laneStatus = {};
        for (i = 0; i < lanes.length; i++) {
            startStep[lanes[i].id] = 0;
            laneStatus[lanes[i].id] = "running";
        }
        for (var s0 = 0; s0 < steps.length; s0++) {
            var laneChange0 = steps[s0].lane || {};
            for (var lid0 in laneChange0) {
                if (laneChange0[lid0] === "new" && startStep[lid0] === 0) startStep[lid0] = s0;
            }
        }

        var rows = [];
        var yCursor = headerH + 16 * widthScale;
        for (var r = 0; r < steps.length; r++) {
            var rowOps = seqOps(steps[r]);
            var maxLines = 1;
            for (var ri = 0; ri < rowOps.length; ri++) {
                if (rowOps[ri].text !== undefined) {
                    maxLines = Math.max(maxLines, lineCount(rowOps[ri].text));
                }
            }
            var atInfo = {};
            var maxAtH = 0;
            for (var ai0 = 0; ai0 < rowOps.length; ai0++) {
                if (rowOps[ai0].at === undefined) continue;
                var aiLane = laneIndex(lanes, rowOps[ai0].at);
                var atLines = lineCount(rowOps[ai0].text || "");
                var atBW = Math.min(widths[aiLane] - 8,
                    Math.max(maxLineWidth(rowOps[ai0].text || "", font, false) + 20, 48));
                var atBH = 22 + (atLines - 1) * lineH;
                atInfo[aiLane] = { bw: atBW, bh: atBH };
                maxAtH = Math.max(maxAtH, atBH);
            }

            /* 先按节点中心估出箭头标签的 x，再决定标签上下分层。 */
            var arrowSpecs = [];
            for (var as = 0; as < rowOps.length; as++) {
                var aop = rowOps[as];
                if (aop.from === undefined || aop.to === undefined) continue;
                var afi = laneIndex(lanes, aop.from);
                var ati = laneIndex(lanes, aop.to);
                var ax1 = centers[afi];
                var ax2 = centers[ati];
                var adir = ax2 >= ax1 ? 1 : -1;
                if (atInfo[afi]) ax1 = centers[afi] + adir * atInfo[afi].bw / 2;
                if (atInfo[ati]) ax2 = centers[ati] - adir * atInfo[ati].bw / 2;
                var alw = maxLineWidth(aop.text || "", labelFont, false) + 10;
                arrowSpecs.push({
                    op: aop, fi: afi, ti: ati, x1: ax1, x2: ax2,
                    lw: alw, level: 0
                });
            }
            var levels = [];
            for (var ari = 0; ari < arrowSpecs.length; ari++) {
                var spec = arrowSpecs[ari];
                var level = 0;
                while (true) {
                    if (!levels[level]) levels[level] = [];
                    var cxSpec = (spec.x1 + spec.x2) / 2;
                    var overlap = false;
                    for (var lv = 0; lv < levels[level].length; lv++) {
                        var other = levels[level][lv];
                        var otherCx = (other.x1 + other.x2) / 2;
                        if (Math.abs(cxSpec - otherCx) < (spec.lw + other.lw) / 2 + 2) {
                            overlap = true;
                            break;
                        }
                    }
                    if (!overlap) break;
                    level += 1;
                }
                spec.level = level;
                levels[level].push(spec);
            }

            var labH = 17 * widthScale;
            var aboveBase = maxAtH > 0 ?
                maxAtH / 2 + 2 * widthScale + labH : 18 * widthScale;
            var belowBase = maxAtH > 0 ?
                maxAtH / 2 + 2 * widthScale : 2 * widthScale;
            var topExtra = 0;
            var bottomExtra = 0;
            for (var exi = 0; exi < arrowSpecs.length; exi++) {
                var exSpec = arrowSpecs[exi];
                if (exSpec.level % 2 === 0) {
                    var aboveR = aboveBase + (exSpec.level / 2) * 16 * widthScale;
                    topExtra = Math.max(topExtra, aboveR - baseRow / 2);
                } else {
                    var belowR = belowBase + ((exSpec.level - 1) / 2) * 16 * widthScale + labH;
                    bottomExtra = Math.max(bottomExtra, belowR - baseRow / 2);
                }
            }
            topExtra = Math.max(0, topExtra);
            bottomExtra = Math.max(0, bottomExtra);
            var contentH = baseRow + (maxLines - 1) * lineH;
            var rowHeight = topExtra + contentH + bottomExtra;
            var lineY = yCursor + topExtra + contentH / 2;
            var row = {
                index: r,
                top: yCursor,
                height: rowHeight,
                center: lineY,
                ops: [],
                statusBefore: {},
                statusAfter: {}
            };
            for (i = 0; i < lanes.length; i++) row.statusBefore[lanes[i].id] = laneStatus[lanes[i].id];
            var laneChange = steps[r].lane || {};
            for (var lid in laneChange) {
                if (laneChange[lid] === "new") laneStatus[lid] = "running";
                else laneStatus[lid] = laneChange[lid];
            }
            for (i = 0; i < lanes.length; i++) row.statusAfter[lanes[i].id] = laneStatus[lanes[i].id];

            var arrowIndex = 0;
            for (var oi2 = 0; oi2 < rowOps.length; oi2++) {
                var op2 = rowOps[oi2];
                var tone = op2.tone || "";
                if (op2.at !== undefined) {
                    var ai = laneIndex(lanes, op2.at);
                    var info = atInfo[ai];
                    row.ops.push({
                        type: "at", lane: ai, x: centers[ai] - info.bw / 2,
                        y: lineY - info.bh / 2, w: info.bw, h: info.bh,
                        text: op2.text || "", font: font, tone: tone
                    });
                    addText(texts, op2.text || "", centers[ai], lineY, font,
                        info.bw - 10, false, "middle",
                        { blockId: "seq:" + r + ":" + ai + ":at", box: row.ops[row.ops.length - 1] });
                } else if (op2.span !== undefined) {
                    var sidx = op2.span.map(function (id) { return laneIndex(lanes, id); });
                    var slo = Math.min.apply(null, sidx);
                    var shi = Math.max.apply(null, sidx);
                    var sx1 = left[slo] + 3;
                    var sx2 = left[shi] + widths[shi] - 3;
                    row.ops.push({
                        type: "span", x: sx1, y: lineY - 8, w: sx2 - sx1, h: 16,
                        text: op2.text || "", font: Math.max(11, font - 1), tone: tone
                    });
                    addText(texts, op2.text || "", (sx1 + sx2) / 2, lineY,
                        Math.max(11, font - 1), sx2 - sx1 - 10, false, "middle",
                        { blockId: "seq:" + r + ":span", box: row.ops[row.ops.length - 1] });
                } else if (op2.from !== undefined && op2.to !== undefined) {
                    var aspec = arrowSpecs[arrowIndex++];
                    var labelY;
                    if (aspec.level % 2 === 0) {
                        labelY = lineY - aboveBase - (aspec.level / 2) * 16 * widthScale;
                    } else {
                        labelY = lineY + belowBase + ((aspec.level - 1) / 2) * 16 * widthScale;
                    }
                    var labelX = (aspec.x1 + aspec.x2) / 2 - aspec.lw / 2;
                    row.ops.push({
                        type: "arrow", x1: aspec.x1, y1: lineY, x2: aspec.x2, y2: lineY,
                        label: {
                            x: labelX, y: labelY, w: aspec.lw, h: labH,
                            text: op2.text || "", cx: (aspec.x1 + aspec.x2) / 2,
                            cy: labelY + labH / 2, level: aspec.level
                        },
                        font: labelFont, tone: tone, dashed: !!op2.dashed
                    });
                    addText(texts, op2.text || "", (aspec.x1 + aspec.x2) / 2,
                        labelY + labH / 2, labelFont, aspec.lw - 8, false, "middle");
                }
            }
            if (steps[r].label) {
                addText(texts, steps[r].label, margin + labelColW - 8, lineY,
                    labelFont, Math.max(20, labelColW - 12), false, "end");
            }
            rows.push(row);
            yCursor += rowHeight;
        }
        var lastBottom = yCursor;

        var state = spec.state || [];
        var stateCells = [];
        var stateHeight = 0;
        if (state.length) {
            var panelY = lastBottom + 14 * widthScale;
            var stateFont = Math.max(11, 12 * widthScale);
            var gap = 6;
            var cellH = 22 * widthScale;
            var cx = margin;
            var cy = panelY;
            for (i = 0; i < state.length; i++) {
                var maxValue = state[i].value === undefined ? "" : String(state[i].value);
                for (var sr = 0; sr < steps.length; sr++) {
                    var set = steps[sr].set || {};
                    if (set[state[i].id] !== undefined && String(set[state[i].id]).length > maxValue.length) {
                        maxValue = String(set[state[i].id]);
                    }
                }
                var label = (state[i].label || state[i].id) + " = " + maxValue;
                var cellW = Math.min(width - margin * 2, textWidth(label, stateFont, false) + 18);
                if (cx + cellW > width - margin && cx > margin) {
                    cx = margin;
                    cy += cellH + gap;
                }
                stateCells.push({
                    id: state[i].id, label: state[i].label || state[i].id,
                    x: cx, y: cy, w: cellW, h: cellH, font: stateFont
                });
                addText(texts, label, cx + cellW / 2, cy + cellH / 2,
                    stateFont, cellW - 8, false, "middle",
                    { blockId: "seq:state:" + state[i].id, box: stateCells[stateCells.length - 1] });
                cx += cellW + gap;
            }
            stateHeight = (cy + cellH) - panelY;
        }

        var height = lastBottom + (state.length ? 14 * widthScale + stateHeight + 10 * widthScale : 12 * widthScale);

        var arrowLabels = [];
        for (var rr = 0; rr < rows.length; rr++) {
            for (var oo = 0; oo < rows[rr].ops.length; oo++) {
                var item = rows[rr].ops[oo];
                if (item.type === "arrow") {
                    arrowLabels.push({
                        x: item.label.x, y: item.label.y, w: item.label.w, h: item.label.h,
                        text: item.label.text, step: rr
                    });
                }
            }
        }

        return {
            kind: "seq", width: width, height: height, margin: margin,
            headerH: headerH, lanes: lanes, laneWidths: widths, left: left, centers: centers,
            rows: rows, lastBottom: lastBottom, state: state, stateCells: stateCells,
            stateFont: stateHeight ? Math.max(11, 12 * widthScale) : 12,
            font: font, labelFont: labelFont, lineH: lineH, scale: widthScale,
            startStep: startStep, statusBefore: statusBefore, statusAfter: statusAfter,
            arrowLabels: arrowLabels, labelColW: labelColW, hasRowLabel: hasRowLabel,
            texts: texts
        };
    }

    function laneIndex(lanes, id) {
        for (var i = 0; i < lanes.length; i++) {
            if (lanes[i].id === id) return i;
        }
        throw new Error("unknown lane: " + id);
    }

    /* ------------------------------------------------------------------ *
     * grid
     * ------------------------------------------------------------------ */

    function bracketRange(value, limit) {
        return parseRange(value, limit, "column");
    }

    function cellFontFor(value, cell) {
        if (value === null || value === undefined || String(value) === "") return 10;
        var units = textWidth(String(value), 1, true);
        return Math.min(10, Math.max(5, (cell - 5) / Math.max(units, 0.1)));
    }

    function layoutGrid(spec) {
        var rows = spec.rows || [];
        var cols = spec.cols || 0;
        var margin = 12;
        var texts = [];
        var cell = cols > 20 ? 22 : 30;
        var rowLabelW = 52;
        for (var i = 0; i < rows.length; i++) {
            rowLabelW = Math.max(rowLabelW, maxLineWidth(rows[i].label || "", 12, false) + 18);
        }
        var brackets = spec.brackets || [];
        var topBrackets = brackets.filter(function (b) { return !b.below; });
        var bottomBrackets = brackets.filter(function (b) { return b.below; });
        var topBracketH = topBrackets.length ? 30 : 0;
        var bottomBracketH = bottomBrackets.length ? 30 : 0;

        var width = margin * 2 + rowLabelW + cols * cell;
        if (width > 760) {
            cell = Math.max(16, (760 - margin * 2 - rowLabelW) / cols);
            width = margin * 2 + rowLabelW + cols * cell;
        }
        var top = margin + topBracketH + 20;
        var gridW = cols * cell;
        var gridH = rows.length * cell;
        var legend = spec.legend || [];
        var legendFont = 11;
        var legendItems = [];
        var legendX = margin;
        var legendY = top + gridH + bottomBracketH + 14;
        var legendRowH = 20;
        var legendW = width - margin * 2;
        for (i = 0; i < legend.length; i++) {
            var labelW = textWidth(legend[i].label || legend[i].key, legendFont, false);
            var itemW = 16 + labelW + 16;
            if (legendX + itemW > margin + legendW && legendX > margin) {
                legendX = margin;
                legendY += legendRowH;
            }
            legendItems.push({
                key: legend[i].key, label: legend[i].label || legend[i].key,
                index: i, x: legendX, y: legendY, w: itemW
            });
            addText(texts, legend[i].label || legend[i].key, legendX + 16,
                legendY + 10, legendFont, labelW + 2, false, "start");
            legendX += itemW;
        }
        var legendH = legend.length ? (legendY - (top + gridH + bottomBracketH + 14) + legendRowH) : 0;

        var colLabels = [];
        if (spec.colLabels === "index") {
            for (i = 0; i < cols; i++) colLabels.push(String(i));
        } else if (Array.isArray(spec.colLabels)) {
            colLabels = spec.colLabels;
        }

        var colX = function (c) { return margin + rowLabelW + c * cell; };
        var rowY = function (r) { return top + r * cell; };

        var bracketLayout = [];
        for (i = 0; i < brackets.length; i++) {
            var br = brackets[i];
            var range = bracketRange(br.c, cols);
            var bx1 = colX(Math.max(0, range[0]));
            var bx2 = colX(Math.min(cols - 1, range[1])) + cell;
            var by;
            if (br.below) {
                by = top + gridH + 8;
            } else {
                by = margin + topBracketH - 10;
            }
            bracketLayout.push({
                x1: bx1, x2: bx2, y: by, below: !!br.below,
                label: br.label || "", font: 11
            });
            addText(texts, br.label || "", (bx1 + bx2) / 2,
                br.below ? by + 18 : by - 9, 11, bx2 - bx1 - 4, false, "middle");
        }

        if (colLabels.length) {
            for (i = 0; i < cols; i++) {
                addText(texts, colLabels[i] === undefined ? "" : String(colLabels[i]),
                    colX(i) + cell / 2, top - 8, 10, cell - 2, true, "middle");
            }
        }
        for (i = 0; i < rows.length; i++) {
            addText(texts, rows[i].label || "", margin + rowLabelW - 8,
                rowY(i) + cell / 2, 12, rowLabelW - 12, false, "end");
        }

        var cells = [];
        var initial = spec.cells || [];
        for (var r = 0; r < rows.length; r++) {
            cells.push([]);
            for (var c = 0; c < cols; c++) {
                var value = initial[r] && initial[r][c] !== undefined ? initial[r][c] : null;
                var cellFont = cellFontFor(value, cell);
                if (value !== null && value !== undefined && String(value) !== "") {
                    addText(texts, String(value), colX(c) + cell / 2, rowY(r) + cell / 2,
                        cellFont, cell - 3, true, "middle",
                        { blockId: "grid:" + r + ":" + c, box: { x: colX(c), y: rowY(r), w: cell, h: cell } });
                }
                cells[r].push({ r: r, c: c, x: colX(c), y: rowY(r), w: cell, h: cell,
                    text: value === undefined ? null : value, font: cellFont });
            }
        }

        /* paint 改写的文字也要参与宽度检查。 */
        for (var psi = 0; psi < (spec.steps || []).length; psi++) {
            var paintsForText = spec.steps[psi].paint || [];
            for (var pti = 0; pti < paintsForText.length; pti++) {
                var pt = paintsForText[pti];
                if (pt.text === undefined || pt.text === null) continue;
                var pr = parseRange(pt.r, rows.length, "row");
                var pc = parseRange(pt.c, cols, "column");
                for (var pyr = pr[0]; pyr <= pr[1]; pyr++) {
                    for (var pxc = pc[0]; pxc <= pc[1]; pxc++) {
                        if (pyr < 0 || pyr >= rows.length || pxc < 0 || pxc >= cols) continue;
                        addText(texts, String(pt.text), colX(pxc) + cell / 2,
                            rowY(pyr) + cell / 2, cellFontFor(pt.text, cell), cell - 3,
                            true, "middle",
                            { blockId: "grid:" + pyr + ":" + pxc,
                              box: { x: colX(pxc), y: rowY(pyr), w: cell, h: cell } });
                    }
                }
            }
        }

        var height = top + gridH + (legendH ? bottomBracketH + 14 + legendH : bottomBracketH) + margin;

        return {
            kind: "grid", width: width, height: height, margin: margin,
            cell: cell, rowLabelW: rowLabelW, top: top, gridW: gridW, gridH: gridH,
            rows: rows, cols: cols, colX: colX, rowY: rowY, cells: cells,
            colLabels: colLabels, brackets: bracketLayout, legend: legendItems,
            legendFont: legendFont, texts: texts
        };
    }

    /* ------------------------------------------------------------------ *
     * flow
     * ------------------------------------------------------------------ */

    function pointOnRect(rect, targetX, targetY) {
        var cx = rect.x + rect.w / 2;
        var cy = rect.y + rect.h / 2;
        var dx = targetX - cx;
        var dy = targetY - cy;
        if (Math.abs(dx) < 1e-9 && Math.abs(dy) < 1e-9) return { x: cx, y: cy };
        var sx = dx ? (rect.w / 2) / Math.abs(dx) : Infinity;
        var sy = dy ? (rect.h / 2) / Math.abs(dy) : Infinity;
        var s = Math.min(sx, sy);
        return { x: cx + dx * s, y: cy + dy * s };
    }

    function polylineLength(points) {
        var total = 0;
        for (var i = 1; i < points.length; i++) {
            total += Math.hypot(points[i].x - points[i - 1].x, points[i].y - points[i - 1].y);
        }
        return total;
    }

    function pointAt(points, fraction) {
        var total = polylineLength(points);
        if (total <= 0) return { x: points[0].x, y: points[0].y };
        var target = total * fraction;
        var walked = 0;
        for (var i = 1; i < points.length; i++) {
            var dx = points[i].x - points[i - 1].x;
            var dy = points[i].y - points[i - 1].y;
            var seg = Math.hypot(dx, dy);
            if (walked + seg >= target) {
                var t = seg ? (target - walked) / seg : 0;
                return { x: points[i - 1].x + dx * t, y: points[i - 1].y + dy * t };
            }
            walked += seg;
        }
        return { x: points[points.length - 1].x, y: points[points.length - 1].y };
    }

    function pointAtTangent(points, fraction) {
        var total = polylineLength(points);
        if (total <= 0) {
            return { x: points[0].x, y: points[0].y, dx: 1, dy: 0 };
        }
        var target = total * fraction;
        var walked = 0;
        for (var i = 1; i < points.length; i++) {
            var dx = points[i].x - points[i - 1].x;
            var dy = points[i].y - points[i - 1].y;
            var seg = Math.hypot(dx, dy);
            if (walked + seg >= target) {
                var t = seg ? (target - walked) / seg : 0;
                return {
                    x: points[i - 1].x + dx * t, y: points[i - 1].y + dy * t,
                    dx: dx, dy: dy
                };
            }
            walked += seg;
        }
        var n = points.length;
        return {
            x: points[n - 1].x, y: points[n - 1].y,
            dx: points[n - 1].x - points[n - 2].x,
            dy: points[n - 1].y - points[n - 2].y
        };
    }

    function segmentIntersectsRect(a, b, rect, pad) {
        pad = pad === undefined ? 1.5 : pad;
        var xmin = rect.x + pad;
        var xmax = rect.x + rect.w - pad;
        var ymin = rect.y + pad;
        var ymax = rect.y + rect.h - pad;
        var dx = b.x - a.x;
        var dy = b.y - a.y;
        var p = [-dx, dx, -dy, dy];
        var q = [a.x - xmin, xmax - a.x, a.y - ymin, ymax - a.y];
        var t0 = 0;
        var t1 = 1;
        for (var i = 0; i < 4; i++) {
            if (p[i] === 0) {
                if (q[i] < 0) return false;
            } else {
                var t = q[i] / p[i];
                if (p[i] < 0) {
                    if (t > t1) return false;
                    if (t > t0) t0 = t;
                } else {
                    if (t < t0) return false;
                    if (t < t1) t1 = t;
                }
            }
        }
        return t0 <= t1;
    }

    function orientation(a, b, c) {
        return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    }

    function segmentsCross(a, b, c, d) {
        var eps = 1e-7;
        var d1 = orientation(a, b, c);
        var d2 = orientation(a, b, d);
        var d3 = orientation(c, d, a);
        var d4 = orientation(c, d, b);
        return (((d1 > eps && d2 < -eps) || (d1 < -eps && d2 > eps)) &&
            ((d3 > eps && d4 < -eps) || (d3 < -eps && d4 > eps)));
    }

    function rectsOverlap(a, b, eps) {
        eps = eps === undefined ? 0.01 : eps;
        return a.x < b.x + b.w - eps && b.x < a.x + a.w - eps &&
            a.y < b.y + b.h - eps && b.y < a.y + a.h - eps;
    }

    function layoutFlow(spec) {
        var grid = spec.grid || [1, 1];
        var declaredCols = grid[0];
        var declaredRows = grid[1];
        var cols = declaredCols;
        var rows = declaredRows;
        var gapX = 56;
        var gapY = 48;
        var margin = 16;
        var font = 13;
        var subFont = 11;
        var labelFont = 11;
        var bandFont = 12;
        var slotFont = 10;
        var texts = [];
        var nodesIn = spec.nodes || [];
        var edgesIn = spec.edges || [];
        var maxEdgeLabelW = 0;
        for (var elw = 0; elw < edgesIn.length; elw++) {
            if (edgesIn[elw].label) {
                maxEdgeLabelW = Math.max(maxEdgeLabelW,
                    maxLineWidth(edgesIn[elw].label, labelFont, false) + 10);
            }
        }
        gapX = Math.max(gapX, maxEdgeLabelW + 16);
        var bandsIn = spec.bands || [];
        var usedCols = {};
        var usedRows = {};
        for (var uci = 0; uci < nodesIn.length; uci++) {
            var ucn = nodesIn[uci];
            var uca = ucn.at || [0, 0];
            var ucs = ucn.span || [1, 1];
            for (var ucc = uca[0]; ucc < uca[0] + ucs[0]; ucc++) usedCols[ucc] = true;
            for (var ucr = uca[1]; ucr < uca[1] + ucs[1]; ucr++) usedRows[ucr] = true;
        }
        var colMap = {};
        var rowMap = {};
        var mappedCols = 0;
        var mappedRows = 0;
        for (var dc = 0; dc < declaredCols; dc++) {
            if (usedCols[dc]) { colMap[dc] = mappedCols++; }
        }
        for (var dr = 0; dr < declaredRows; dr++) {
            if (usedRows[dr]) { rowMap[dr] = mappedRows++; }
        }
        if (mappedCols === 0) { mappedCols = 1; colMap[0] = 0; }
        if (mappedRows === 0) { mappedRows = 1; rowMap[0] = 0; }
        cols = mappedCols;
        rows = mappedRows;
        var mappedNodes = [];
        for (var mni = 0; mni < nodesIn.length; mni++) {
            var mn = nodesIn[mni];
            var ma = mn.at || [0, 0];
            var ms = mn.span || [1, 1];
            var mapped = {};
            for (var mk in mn) mapped[mk] = mn[mk];
            mapped.at = [colMap[ma[0]], rowMap[ma[1]]];
            mappedNodes.push(mapped);
        }
        nodesIn = mappedNodes;
        var bands = [];
        for (var bi0 = 0; bi0 < bandsIn.length; bi0++) {
            var b0 = bandsIn[bi0];
            var br = b0.rows || [0, 0];
            var bRows = [];
            for (var bri = br[0]; bri <= br[1]; bri++) {
                if (rowMap[bri] !== undefined) bRows.push(rowMap[bri]);
            }
            if (bRows.length) {
                bands.push({ label: b0.label, rows: [Math.min.apply(null, bRows), Math.max.apply(null, bRows)] });
            }
        }
        var bandW = 0;
        for (var b = 0; b < bands.length; b++) {
            bandW = Math.max(bandW, maxLineWidth(bands[b].label || "", bandFont, false) + 22);
        }

        var queueContents = {};
        var queueWidth = {};
        var subWidths = {};
        var subLinesByNode = {};
        for (var si = 0; si < (spec.steps || []).length; si++) {
            var qmap = spec.steps[si].queue || {};
            for (var qid in qmap) {
                queueContents[qid] = qmap[qid];
                for (var qi = 0; qi < qmap[qid].length; qi++) {
                    queueWidth[qid] = Math.max(queueWidth[qid] || 0,
                        textWidth(String(qmap[qid][qi]), 10, false) + 22);
                }
            }
            var smap = spec.steps[si].set || {};
            for (var sid0 in smap) {
                subWidths[sid0] = Math.max(subWidths[sid0] || 0,
                    maxLineWidth(smap[sid0], subFont, false));
                subLinesByNode[sid0] = Math.max(subLinesByNode[sid0] || 0,
                    lineCount(smap[sid0]));
            }
        }
        for (var ni0 = 0; ni0 < nodesIn.length; ni0++) {
            if (nodesIn[ni0].sub) {
                subWidths[nodesIn[ni0].id] = Math.max(subWidths[nodesIn[ni0].id] || 0,
                    maxLineWidth(nodesIn[ni0].sub, subFont, false));
                subLinesByNode[nodesIn[ni0].id] = Math.max(subLinesByNode[nodesIn[ni0].id] || 0,
                    lineCount(nodesIn[ni0].sub));
            }
        }

        function recShape(node) {
            return node.shape || "box";
        }

        function nodeSize(node) {
            var lw = textWidth(node.label || "", font, false);
            var sw = subWidths[node.id] || 0;
            var labelLines = lineCount(node.label || "");
            var subLines = subLinesByNode[node.id] || 0;
            var labelH = labelLines * font * 1.22;
            var subH = subLines * subFont * 1.22;
            var contentH = labelH + (subLines ? 4 + subH : 0);
            var w = Math.max(76, Math.max(lw, sw) + 26);
            var h = Math.max(34, contentH + 16);
            if (node.shape === "queue") {
                var slots = node.slots || 4;
                w = Math.max(w, slots * 22 + 12);
                h = Math.max(h, 8 + labelH + (subLines ? 4 + subH : 0) + 6 + 20 + 8);
            } else if (node.shape === "store") {
                h = Math.max(h, 44);
            } else if (node.shape === "note") {
                w = Math.max(w, lw + 8);
                h = Math.max(24, contentH + 12);
            }
            w = Math.max(w, queueWidth[node.id] || 0);
            return {
                w: w, h: h, labelLines: labelLines, subLines: subLines,
                labelH: labelH, subH: subH, contentH: contentH
            };
        }

        var colW = [];
        var rowH = [];
        for (var c = 0; c < cols; c++) colW.push(76);
        for (var r = 0; r < rows; r++) rowH.push(40);
        for (var ni = 0; ni < nodesIn.length; ni++) {
            var nd = nodesIn[ni];
            var size = nodeSize(nd);
            var at = nd.at || [0, 0];
            var span = nd.span || [1, 1];
            if (span[0] === 1) colW[at[0]] = Math.max(colW[at[0]], size.w);
            if (span[1] === 1) rowH[at[1]] = Math.max(rowH[at[1]], size.h);
        }
        for (var pass = 0; pass < 3; pass++) {
            for (var nj = 0; nj < nodesIn.length; nj++) {
                var n2 = nodesIn[nj];
                var sz = nodeSize(n2);
                var a2 = n2.at || [0, 0];
                var sp = n2.span || [1, 1];
                var haveW = 0;
                for (var cc = a2[0]; cc < a2[0] + sp[0]; cc++) haveW += colW[cc];
                haveW += gapX * (sp[0] - 1);
                if (haveW < sz.w) {
                    var addW = (sz.w - haveW) / sp[0];
                    for (var c2 = a2[0]; c2 < a2[0] + sp[0]; c2++) colW[c2] += addW;
                }
                var haveH = 0;
                for (var rr = a2[1]; rr < a2[1] + sp[1]; rr++) haveH += rowH[rr];
                haveH += gapY * (sp[1] - 1);
                if (haveH < sz.h) {
                    var addH = (sz.h - haveH) / sp[1];
                    for (var r2 = a2[1]; r2 < a2[1] + sp[1]; r2++) rowH[r2] += addH;
                }
            }
        }

        var width = margin * 2 + bandW + colW.reduce(function (a, b2) { return a + b2; }, 0) +
            gapX * (cols - 1);
        var height = margin * 2 + rowH.reduce(function (a, b2) { return a + b2; }, 0) +
            gapY * (rows - 1);
        if (width > 760) {
            var scale = 760 / width;
            width = 760;
            for (var cs = 0; cs < colW.length; cs++) colW[cs] *= scale;
            for (var rs = 0; rs < rowH.length; rs++) rowH[rs] *= scale;
            bandW *= scale;
            margin *= scale;
            gapX *= scale;
            gapY *= scale;
            font *= scale;
            subFont *= scale;
            labelFont *= scale;
            bandFont *= scale;
            slotFont *= scale;
            height = margin * 2 + rowH.reduce(function (a, b2) { return a + b2; }, 0) +
                gapY * (rows - 1);
        }

        var colStart = [];
        var xx = margin + bandW;
        for (c = 0; c < cols; c++) {
            colStart.push(xx);
            xx += colW[c] + gapX;
        }
        var rowStart = [];
        var yy = margin;
        for (r = 0; r < rows; r++) {
            rowStart.push(yy);
            yy += rowH[r] + gapY;
        }

        var nodesById = {};
        var nodeRects = [];
        var nodeBlocks = [];
        for (ni = 0; ni < nodesIn.length; ni++) {
            var node = nodesIn[ni];
            var a = node.at || [0, 0];
            var s = node.span || [1, 1];
            var nx = colStart[a[0]];
            var ny = rowStart[a[1]];
            var nw = 0;
            var nh = 0;
            for (c = a[0]; c < a[0] + s[0]; c++) nw += colW[c];
            nw += gapX * (s[0] - 1);
            for (r = a[1]; r < a[1] + s[1]; r++) nh += rowH[r];
            nh += gapY * (s[1] - 1);
            var metrics = nodeSize(node);
            var labelH = metrics.labelH;
            var subH = metrics.subH;
            var contentH = metrics.contentH;
            var labelY;
            var subY;
            var slotY;
            if (recShape(node) === "queue") {
                labelY = ny + 8 + labelH / 2;
                subY = ny + 8 + labelH + 4 + subH / 2;
                slotY = ny + 8 + labelH + (subH ? 4 + subH : 0) + 6;
            } else {
                var contentTop = ny + (nh - contentH) / 2;
                labelY = contentTop + labelH / 2;
                subY = contentTop + labelH + 4 + subH / 2;
                slotY = 0;
            }
            var rec = {
                id: node.id, label: node.label || "", sub: node.sub || "",
                shape: recShape(node), slots: node.slots || 4,
                hidden: !!node.hidden, x: nx, y: ny, w: nw, h: nh,
                font: font, subFont: subFont, queue: node.id,
                labelY: labelY, subY: subY, slotY: slotY,
                labelH: labelH, subH: subH, labelLines: metrics.labelLines,
                subLines: metrics.subLines
            };
            nodesById[node.id] = rec;
            nodeRects.push(rec);
            nodeBlocks.push({
                nodeId: rec.id, block: "label", x: nx, y: labelY - labelH / 2,
                w: nw, h: labelH
            });
            addText(texts, rec.label, nx + nw / 2, labelY, font, nw - 16, false, "middle",
                { blockId: rec.id + ":label", box: rec });
            if (rec.sub) {
                nodeBlocks.push({
                    nodeId: rec.id, block: "sub", x: nx, y: subY - subH / 2,
                    w: nw, h: subH
                });
                addText(texts, rec.sub, nx + nw / 2, subY, subFont, nw - 16, false, "middle",
                    { blockId: rec.id + ":sub", box: rec });
            }
            if (rec.shape === "queue") {
                var slotW = (nw - 12) / rec.slots;
                for (var sl = 0; sl < rec.slots; sl++) {
                    addText(texts, "", nx + 6 + slotW * (sl + 0.5), slotY + 10, slotFont,
                        slotW - 4, false, "middle");
                }
            }
        }

        /* 所有步骤里出现过的 sub / 队列内容都参与宽度检查。 */
        for (si = 0; si < (spec.steps || []).length; si++) {
            var stepForText = spec.steps[si] || {};
            var setForText = stepForText.set || {};
            for (var setNid in setForText) {
                var setNode = nodesById[setNid];
                if (!setNode) continue;
                addText(texts, setForText[setNid], setNode.x + setNode.w / 2,
                    setNode.subY, subFont, setNode.w - 16, false, "middle",
                    { blockId: setNode.id + ":sub", box: setNode });
            }
            var queueForText = stepForText.queue || {};
            for (var queueNid in queueForText) {
                var queueNode = nodesById[queueNid];
                if (!queueNode || queueNode.shape !== "queue") continue;
                var textSlotW = (queueNode.w - 12) / queueNode.slots;
                for (var qti = 0; qti < queueForText[queueNid].length; qti++) {
                    addText(texts, queueForText[queueNid][qti],
                        queueNode.x + 6 + textSlotW * (qti + 0.5), queueNode.slotY + 10,
                        10, textSlotW - 4, false, "middle");
                }
            }
        }

        var bandLayout = [];
        for (b = 0; b < bands.length; b++) {
            var br0 = bands[b].rows || [0, 0];
            var y0 = Math.max(margin, rowStart[br0[0]] - gapY / 2);
            var y1 = rowStart[br0[1]] + rowH[br0[1]] + gapY / 2;
            bandLayout.push({
                label: bands[b].label || "", x: margin, y: y0,
                w: width - margin * 2, h: Math.min(height - margin, y1) - y0, index: b
            });
            addText(texts, bands[b].label || "", margin + 6, (y0 + Math.min(height - margin, y1)) / 2,
                bandFont, Math.max(20, bandW - 12), false, "start");
        }

        var edgesById = {};
        var edgeLayout = [];
        var validEdges = [];
        var pairGroups = {};
        for (var eidx = 0; eidx < edgesIn.length; eidx++) {
            var ein = edgesIn[eidx];
            if (!nodesById[ein.from] || !nodesById[ein.to]) continue;
            var pkey = [ein.from, ein.to].sort().join("|");
            if (!pairGroups[pkey]) pairGroups[pkey] = [];
            pairGroups[pkey].push(ein);
            validEdges.push(ein);
        }
        var parallelIndex = {};
        var parallelCount = {};
        var pairKeyOf = {};
        for (var pk in pairGroups) {
            var group = pairGroups[pk];
            group.sort(function (a, b) { return a.id < b.id ? -1 : (a.id > b.id ? 1 : 0); });
            for (var gi = 0; gi < group.length; gi++) {
                parallelIndex[group[gi].id] = gi;
                parallelCount[group[gi].id] = group.length;
                pairKeyOf[group[gi].id] = pk;
            }
        }

        function chooseSides(edge, route) {
            var from = nodesById[edge.from];
            var to = nodesById[edge.to];
            var dx = (to.x + to.w / 2) - (from.x + from.w / 2);
            var dy = (to.y + to.h / 2) - (from.y + from.h / 2);
            var fs = edge.fromSide;
            var ts = edge.toSide;
            if (!fs) {
                if (route === "hv") fs = dx >= 0 ? "right" : "left";
                else if (route === "vh") fs = dy >= 0 ? "bottom" : "top";
                else if (Math.abs(dx) >= Math.abs(dy)) fs = dx >= 0 ? "right" : "left";
                else fs = dy >= 0 ? "bottom" : "top";
            }
            if (!ts) {
                if (route === "hv") ts = dx >= 0 ? "left" : "right";
                else if (route === "vh") ts = dy >= 0 ? "top" : "bottom";
                else if (Math.abs(dx) >= Math.abs(dy)) ts = dx >= 0 ? "left" : "right";
                else ts = dy >= 0 ? "top" : "bottom";
            }
            return { fromSide: fs, toSide: ts };
        }

        function sideKey(nodeId, side) {
            return nodeId + "|" + side;
        }

        function portPoint(node, side, index, count) {
            var pad = 3;
            if (side === "left" || side === "right") {
                var y = node.y + pad + (node.h - pad * 2) * (index + 1) / (count + 1);
                return { x: side === "left" ? node.x : node.x + node.w, y: y };
            }
            var x = node.x + pad + (node.w - pad * 2) * (index + 1) / (count + 1);
            return { x: x, y: side === "top" ? node.y : node.y + node.h };
        }

        function buildAssignment(baseRoute) {
            var assignment = [];
            for (var ai = 0; ai < validEdges.length; ai++) {
                var edge = validEdges[ai];
                var route = edge.route && edge.route !== "auto" ? edge.route : baseRoute;
                var sides = chooseSides(edge, route);
                assignment.push({
                    edge: edge, route: route,
                    fromSide: sides.fromSide, toSide: sides.toSide
                });
            }
            return assignment;
        }

        function buildPorts(assignment) {
            var sides = {};
            for (var ai = 0; ai < assignment.length; ai++) {
                var a = assignment[ai];
                var fkey = sideKey(a.edge.from, a.fromSide);
                var tkey = sideKey(a.edge.to, a.toSide);
                if (!sides[fkey]) sides[fkey] = [];
                if (!sides[tkey]) sides[tkey] = [];
                sides[fkey].push({ edge: a.edge, end: "from", pi: parallelIndex[a.edge.id] || 0, pk: pairKeyOf[a.edge.id] || "" });
                sides[tkey].push({ edge: a.edge, end: "to", pi: parallelIndex[a.edge.id] || 0, pk: pairKeyOf[a.edge.id] || "" });
            }
            var ports = {};
            for (var key in sides) {
                var list = sides[key];
                list.sort(function (a, b) {
                    if (a.pk !== b.pk) return a.pk < b.pk ? -1 : 1;
                    if (a.pi !== b.pi) return a.pi - b.pi;
                    return a.edge.id < b.edge.id ? -1 : (a.edge.id > b.edge.id ? 1 : 0);
                });
                for (var li = 0; li < list.length; li++) {
                    var item = list[li];
                    var node = nodesById[item.edge[item.end]];
                    var side = item.end === "from" ?
                        assignment.filter(function (x) { return x.edge === item.edge; })[0].fromSide :
                        assignment.filter(function (x) { return x.edge === item.edge; })[0].toSide;
                    if (!ports[item.edge.id]) ports[item.edge.id] = {};
                    ports[item.edge.id][item.end] = portPoint(node, side, li, list.length);
                }
            }
            return ports;
        }

        function buildPath(a, ports) {
            var p1 = ports[a.edge.id].from;
            var p2 = ports[a.edge.id].to;
            var count = parallelCount[a.edge.id] || 1;
            var pi = parallelIndex[a.edge.id] || 0;
            var off = (pi - (count - 1) / 2) * 10;
            var sideGap = 20;
            var midSide = (p1.x + p2.x) / 2;
            if (a.fromSide === "bottom" && a.toSide === "bottom") {
                var downY = Math.max(p1.y, p2.y) + sideGap + off;
                return [p1, { x: p1.x, y: downY }, { x: midSide, y: downY },
                    { x: p2.x, y: downY }, p2];
            }
            if (a.fromSide === "top" && a.toSide === "top") {
                var upY = Math.min(p1.y, p2.y) - sideGap + off;
                return [p1, { x: p1.x, y: upY }, { x: midSide, y: upY },
                    { x: p2.x, y: upY }, p2];
            }
            if (a.fromSide === "left" && a.toSide === "left") {
                var leftX = Math.min(p1.x, p2.x) - sideGap + off;
                var midLeftY = (p1.y + p2.y) / 2;
                return [p1, { x: leftX, y: p1.y }, { x: leftX, y: midLeftY },
                    { x: leftX, y: p2.y }, p2];
            }
            if (a.fromSide === "right" && a.toSide === "right") {
                var rightX = Math.max(p1.x, p2.x) + sideGap + off;
                var midRightY = (p1.y + p2.y) / 2;
                return [p1, { x: rightX, y: p1.y }, { x: rightX, y: midRightY },
                    { x: rightX, y: p2.y }, p2];
            }
            if (a.route === "hv") {
                var mids = [(p1.x + p2.x) / 2 + off];
                for (var hc = 0; hc < cols - 1; hc++) {
                    mids.push(colStart[hc] + colW[hc] + gapX / 2);
                }
                mids.push(margin + 4);
                mids.push(width - margin - 4);
                var bestH = null;
                var bestHS = Infinity;
                for (var hmi = 0; hmi < mids.length; hmi++) {
                    var hx = mids[hmi];
                    var hp = [p1, { x: hx, y: p1.y }, { x: hx, y: p2.y }, p2];
                    var hs = nodeHits(a.edge, hp) * 1000 + pathLength(hp) * 0.01;
                    if (hs < bestHS) { bestHS = hs; bestH = hp; }
                }
                return bestH;
            }
            if (a.route === "vh") {
                var midYs = [(p1.y + p2.y) / 2 + off];
                for (var vr = 0; vr < rows - 1; vr++) {
                    midYs.push(rowStart[vr] + rowH[vr] + gapY / 2);
                }
                midYs.push(margin + 4);
                midYs.push(height - margin - 4);
                var bestV = null;
                var bestVS = Infinity;
                for (var vmi = 0; vmi < midYs.length; vmi++) {
                    var vy = midYs[vmi];
                    var vp = [p1, { x: p1.x, y: vy }, { x: p2.x, y: vy }, p2];
                    var vs = nodeHits(a.edge, vp) * 1000 + pathLength(vp) * 0.01;
                    if (vs < bestVS) { bestVS = vs; bestV = vp; }
                }
                return bestV;
            }
            return [p1, p2];
        }

        function pathSegments(points) {
            var segs = [];
            for (var si = 1; si < points.length; si++) {
                segs.push([points[si - 1], points[si]]);
            }
            return segs;
        }

        function sharesNode(a, b) {
            return a.from === b.from || a.from === b.to || a.to === b.from || a.to === b.to;
        }

        function nodeHits(edge, points) {
            var hits = 0;
            var segs = pathSegments(points);
            for (var ni = 0; ni < nodeRects.length; ni++) {
                var node = nodeRects[ni];
                if (node.id === edge.from || node.id === edge.to) continue;
                var hit = false;
                for (var si = 0; si < segs.length; si++) {
                    if (segmentIntersectsRect(segs[si][0], segs[si][1], node, 1.5)) {
                        hit = true;
                        break;
                    }
                }
                if (hit) hits += 1;
            }
            return hits;
        }

        function crossingCount(edge, points, paths) {
            var count = 0;
            var segs = pathSegments(points);
            for (var oid in paths) {
                if (oid === edge.id) continue;
                var other = null;
                for (var k = 0; k < validEdges.length; k++) {
                    if (validEdges[k].id === oid) { other = validEdges[k]; break; }
                }
                if (!other || sharesNode(edge, other)) continue;
                if ((pairKeyOf[edge.id] || "") === (pairKeyOf[other.id] || "")) continue;
                var osegs = pathSegments(paths[oid]);
                var crossed = false;
                for (var a = 0; a < segs.length && !crossed; a++) {
                    for (var b = 0; b < osegs.length; b++) {
                        if (segmentsCross(segs[a][0], segs[a][1], osegs[b][0], osegs[b][1])) {
                            crossed = true;
                            break;
                        }
                    }
                }
                if (crossed) count += 1;
            }
            return count;
        }

        function pathLength(points) {
            return polylineLength(points);
        }

        var autoEdges = [];
        for (var ve = 0; ve < validEdges.length; ve++) {
            if (!validEdges[ve].route || validEdges[ve].route === "auto") autoEdges.push(validEdges[ve]);
        }
        var routeScores = {};
        var routeNames = ["straight", "hv", "vh"];
        for (var rn = 0; rn < routeNames.length; rn++) {
            var baseRoute = routeNames[rn];
            var assignment = buildAssignment(baseRoute);
            var ports = buildPorts(assignment);
            var paths = {};
            for (var pa = 0; pa < assignment.length; pa++) {
                paths[assignment[pa].edge.id] = buildPath(assignment[pa], ports);
            }
            for (var ae = 0; ae < autoEdges.length; ae++) {
                var autoEdge = autoEdges[ae];
                if (!routeScores[autoEdge.id]) routeScores[autoEdge.id] = {};
                var pts = paths[autoEdge.id];
                var score = nodeHits(autoEdge, pts) * 1000 +
                    crossingCount(autoEdge, pts, paths) * 60 + pathLength(pts) * 0.01;
                routeScores[autoEdge.id][baseRoute] = score;
            }
        }

        var chosenRoute = {};
        for (var ce = 0; ce < autoEdges.length; ce++) {
            var ceid = autoEdges[ce].id;
            var bestRoute = "auto";
            var bestScore = Infinity;
            for (var rn2 = 0; rn2 < routeNames.length; rn2++) {
                var candidate = routeScores[ceid][routeNames[rn2]];
                if (candidate < bestScore) {
                    bestScore = candidate;
                    bestRoute = routeNames[rn2];
                }
            }
            chosenRoute[ceid] = bestRoute;
        }
        /* 同一对节点上的自动边用同一种路线，保证平行。 */
        for (var pk2 in pairGroups) {
            var pg = pairGroups[pk2];
            var allAuto = true;
            for (var pgi = 0; pgi < pg.length; pgi++) {
                if (pg[pgi].route && pg[pgi].route !== "auto") allAuto = false;
            }
            if (!allAuto || pg.length < 2) continue;
            var bestSum = Infinity;
            var groupRoute = "straight";
            for (var rn3 = 0; rn3 < routeNames.length; rn3++) {
                var sum = 0;
                for (var pgj = 0; pgj < pg.length; pgj++) {
                    sum += routeScores[pg[pgj].id][routeNames[rn3]];
                }
                if (sum < bestSum) {
                    bestSum = sum;
                    groupRoute = routeNames[rn3];
                }
            }
            for (var pgk = 0; pgk < pg.length; pgk++) chosenRoute[pg[pgk].id] = groupRoute;
        }

        var finalAssignment = buildAssignment("straight");
        for (var fa = 0; fa < finalAssignment.length; fa++) {
            var faEdge = finalAssignment[fa].edge;
            if (!faEdge.route || faEdge.route === "auto") {
                finalAssignment[fa].route = chosenRoute[faEdge.id] || "straight";
                var fSides = chooseSides(faEdge, finalAssignment[fa].route);
                finalAssignment[fa].fromSide = fSides.fromSide;
                finalAssignment[fa].toSide = fSides.toSide;
            }
        }
        var finalPorts = buildPorts(finalAssignment);
        var finalPaths = {};
        for (var fp = 0; fp < finalAssignment.length; fp++) {
            finalPaths[finalAssignment[fp].edge.id] = buildPath(finalAssignment[fp], finalPorts);
        }

        /* 折线可能绕到节点行外：先把所有几何平移到可见区域，再放标签。 */
        var minX = Infinity;
        var minY = Infinity;
        var maxX = -Infinity;
        var maxY = -Infinity;
        for (var bni = 0; bni < nodeRects.length; bni++) {
            var bnr = nodeRects[bni];
            minX = Math.min(minX, bnr.x);
            minY = Math.min(minY, bnr.y);
            maxX = Math.max(maxX, bnr.x + bnr.w);
            maxY = Math.max(maxY, bnr.y + bnr.h);
        }
        for (var bpi in finalPaths) {
            var bpts = finalPaths[bpi];
            for (var bpj = 0; bpj < bpts.length; bpj++) {
                minX = Math.min(minX, bpts[bpj].x);
                minY = Math.min(minY, bpts[bpj].y);
                maxX = Math.max(maxX, bpts[bpj].x);
                maxY = Math.max(maxY, bpts[bpj].y);
            }
        }
        var shiftX = minX < margin ? margin - minX : 0;
        var shiftY = minY < margin ? margin - minY : 0;
        if (shiftX || shiftY) {
            for (var sn = 0; sn < nodeRects.length; sn++) {
                var snr = nodeRects[sn];
                snr.x += shiftX;
                snr.y += shiftY;
                snr.labelY += shiftY;
                snr.subY += shiftY;
                snr.slotY += shiftY;
            }
            for (var sp in finalPaths) {
                for (var spi = 0; spi < finalPaths[sp].length; spi++) {
                    finalPaths[sp][spi].x += shiftX;
                    finalPaths[sp][spi].y += shiftY;
                }
            }
            for (var sband = 0; sband < bandLayout.length; sband++) {
                bandLayout[sband].y += shiftY;
            }
            for (var st = 0; st < texts.length; st++) {
                texts[st].x += shiftX;
                texts[st].y += shiftY;
                if (texts[st].blockTop !== undefined) {
                    texts[st].blockTop += shiftY;
                    texts[st].blockBottom += shiftY;
                }
                if (texts[st].boxX !== undefined) {
                    texts[st].boxX += shiftX;
                    texts[st].boxY += shiftY;
                }
            }
            for (var sbl = 0; sbl < nodeBlocks.length; sbl++) {
                nodeBlocks[sbl].x += shiftX;
                nodeBlocks[sbl].y += shiftY;
            }
            width += shiftX;
            height += shiftY;
        }
        maxX += shiftX;
        maxY += shiftY;
        if (maxX > width - margin) width += maxX - (width - margin);
        if (maxY > height - margin) height += maxY - (height - margin);
        for (var bnd = 0; bnd < bandLayout.length; bnd++) {
            bandLayout[bnd].x = margin;
            bandLayout[bnd].w = width - margin * 2;
        }

        var tokenText = {};
        var hasToken = {};
        for (var sti = 0; sti < (spec.steps || []).length; sti++) {
            var tlist = spec.steps[sti].token || [];
            for (var tti = 0; tti < tlist.length; tti++) {
                hasToken[tlist[tti].edge] = true;
                if (tlist[tti].label !== undefined && tlist[tti].label !== null) {
                    var oldW = tokenText[tlist[tti].edge] || "";
                    if (textWidth(String(tlist[tti].label), labelFont, false) >
                        textWidth(oldW, labelFont, false)) {
                        tokenText[tlist[tti].edge] = String(tlist[tti].label);
                    }
                }
            }
        }

        /* token 固定放在边的 60% 处、向一侧偏 8px；标签放置时避开它。
         * prefers-reduced-motion 下不移动，因此位置就是这里算出的点。 */
        var tokenDrafts = {};
        for (var tde = 0; tde < finalAssignment.length; tde++) {
            var tedge = finalAssignment[tde].edge;
            if (!hasToken[tedge.id]) continue;
            var ttext = tokenText[tedge.id] || "";
            var tw = textWidth(ttext, labelFont, false) + 10;
            var tinfo = pointAtTangent(finalPaths[tedge.id], 0.6);
            var tlen = Math.hypot(tinfo.dx, tinfo.dy) || 1;
            var tnx = -tinfo.dy / tlen;
            var tny = tinfo.dx / tlen;
            var chosenToken = null;
            for (var tsgn = 1; tsgn >= -1 && !chosenToken; tsgn -= 2) {
                var ttx = tinfo.x + tnx * 8 * tsgn;
                var tty = tinfo.y + tny * 8 * tsgn;
                var trc = { x: ttx - tw / 2, y: tty - 8, w: tw, h: 16, text: ttext };
                if (trc.x < margin || trc.x + trc.w > width - margin ||
                    trc.y < margin || trc.y + trc.h > height - margin) continue;
                var tbad = false;
                for (var tnode = 0; tnode < nodeRects.length; tnode++) {
                    if (rectsOverlap(trc, nodeRects[tnode])) { tbad = true; break; }
                }
                if (!tbad) chosenToken = trc;
            }
            if (!chosenToken) {
                chosenToken = {
                    x: clamp(tinfo.x + tnx * 8, margin + tw / 2 + 1, width - margin - tw / 2 - 1),
                    y: clamp(tinfo.y + tny * 8, margin + 9, height - margin - 9),
                    w: tw, h: 16, text: ttext
                };
            }
            tokenDrafts[tedge.id] = chosenToken;
        }

        var placedLabels = [];
        var pathsById = finalPaths;
        for (var fe = 0; fe < finalAssignment.length; fe++) {
            var fa2 = finalAssignment[fe];
            var edge = fa2.edge;
            var points = finalPaths[edge.id];
            var labelText = edge.label || "";
            var labelLines = splitLines(labelText);
            var labelW = labelText ? maxLineWidth(labelText, labelFont, false) + 10 : 0;
            var labelH = labelText ? Math.max(17, labelLines.length * 14 + 3) : 0;
            var label = {
                x: 0, y: 0, w: labelW, h: labelH, text: labelText,
                cx: 0, cy: 0, error: false
            };
            if (labelText) {
                var fractions = [0.5, 0.42, 0.58, 0.35, 0.65, 0.28, 0.72, 0.22, 0.78, 0.16, 0.84];
                var placed = null;
                for (var fi = 0; fi < fractions.length && !placed; fi++) {
                    var pt = pointAt(points, fractions[fi]);
                    var variants = [
                        { x: pt.x - labelW / 2, y: pt.y - labelH / 2 },
                        { x: pt.x - labelW / 2, y: pt.y - labelH - 4 },
                        { x: pt.x - labelW / 2, y: pt.y + 4 },
                        { x: pt.x - labelW - 10, y: pt.y - labelH / 2 },
                        { x: pt.x + 10, y: pt.y - labelH / 2 }
                    ];
                    for (var vi = 0; vi < variants.length && !placed; vi++) {
                        var lr = {
                            x: clamp(variants[vi].x, margin + 2, width - margin - labelW - 2),
                            y: clamp(variants[vi].y, margin + 2, height - margin - labelH - 2),
                            w: labelW, h: labelH, text: labelText,
                            cx: 0, cy: 0
                        };
                        lr.cx = lr.x + labelW / 2;
                        lr.cy = lr.y + labelH / 2;
                        var bad = false;
                        for (var ln = 0; ln < nodeRects.length; ln++) {
                            if (rectsOverlap(lr, nodeRects[ln])) { bad = true; break; }
                        }
                        if (!bad) {
                            for (var pl = 0; pl < placedLabels.length; pl++) {
                                if (rectsOverlap(lr, placedLabels[pl])) { bad = true; break; }
                            }
                        }
                        if (!bad) {
                            for (var td in tokenDrafts) {
                                if (rectsOverlap(lr, tokenDrafts[td])) { bad = true; break; }
                            }
                        }
                        if (!bad) {
                            for (var pe in pathsById) {
                                if (pe === edge.id) continue;
                                var psegs = pathSegments(pathsById[pe]);
                                for (var ps = 0; ps < psegs.length; ps++) {
                                    if (segmentIntersectsRect(psegs[ps][0], psegs[ps][1], lr, 0)) {
                                        bad = true;
                                        break;
                                    }
                                }
                                if (bad) break;
                            }
                        }
                        if (!bad) {
                            placed = lr;
                            break;
                        }
                    }
                }
                if (!placed) {
                    var mid = pointAt(points, 0.5);
                    placed = {
                        x: clamp(mid.x - labelW / 2, margin + 2, width - margin - labelW - 2),
                        y: clamp(mid.y - labelH / 2, margin + 2, height - margin - labelH - 2),
                        w: labelW, h: labelH, text: labelText, cx: 0, cy: 0
                    };
                    placed.cx = placed.x + labelW / 2;
                    placed.cy = placed.y + labelH / 2;
                    placed.error = true;
                }
                label = placed;
                placedLabels.push(placed);
            }

            var tokenBest = tokenDrafts[edge.id] ||
                { x: 0, y: 0, w: 0, h: 0, text: "" };
            if (tokenBest.text) {
                addText(texts, tokenBest.text, tokenBest.x, tokenBest.y, labelFont,
                    Math.max(10, tokenBest.w - 8), false, "middle");
            }
            var recEdge = {
                id: edge.id, from: edge.from, to: edge.to, points: points,
                route: fa2.route, fromSide: fa2.fromSide, toSide: fa2.toSide,
                dashed: !!edge.dashed, arrow: edge.arrow || "end",
                hidden: !!edge.hidden, label: label, token: tokenBest,
                parallelIndex: parallelIndex[edge.id] || 0,
                parallelCount: parallelCount[edge.id] || 1,
                font: labelFont
            };
            edgesById[edge.id] = recEdge;
            edgeLayout.push(recEdge);
            if (labelText) {
                for (var lli = 0; lli < labelLines.length; lli++) {
                    addText(texts, labelLines[lli], label.cx,
                        label.cy + (lli - (labelLines.length - 1) / 2) * 14,
                        labelFont, Math.max(10, labelW - 8), false, "middle",
                        { blockId: "flow:" + edge.id + ":label", box: label });
                }
            }
        }

        return {
            kind: "flow", width: width, height: height, margin: margin,
            cols: cols, rows: rows, gapX: gapX, gapY: gapY,
            colW: colW, rowH: rowH, colStart: colStart, rowStart: rowStart,
            nodes: nodeRects, nodesById: nodesById, edges: edgeLayout, edgesById: edgesById,
            bands: bandLayout, bandW: bandW, font: font, subFont: subFont,
            labelFont: labelFont, bandFont: bandFont, slotFont: slotFont,
            nodeBlocks: nodeBlocks, texts: texts
        };
    }

    /* ------------------------------------------------------------------ *
     * chart
     * ------------------------------------------------------------------ */

    function niceStep(range, target) {
        var raw = range / Math.max(1, target);
        var power = Math.pow(10, Math.floor(Math.log(raw) / Math.LN10));
        var base = raw / power;
        var step;
        if (base <= 1) step = 1;
        else if (base <= 2) step = 2;
        else if (base <= 5) step = 5;
        else step = 10;
        return step * power;
    }

    function wrapChartLabel(text, maxWidth, fontSize, maxLines) {
        var chars = String(text).split("");
        var lines = [];
        var current = "";
        for (var i = 0; i < chars.length; i++) {
            var candidate = current + chars[i];
            if (current === "" || textWidth(candidate, fontSize, false) <= maxWidth) {
                current = candidate;
            } else if (lines.length < maxLines - 1) {
                lines.push(current);
                current = chars[i];
            } else {
                current = candidate;
            }
        }
        if (current !== "" || !lines.length) lines.push(current);
        return lines;
    }

    function niceXTicks(min, max, logScale) {
        if (logScale) {
            var lo = Math.floor(Math.log2(min));
            var hi = Math.ceil(Math.log2(max));
            var powers = [];
            for (var p = lo; p <= hi; p++) powers.push(Math.pow(2, p));
            if (powers.length > 10) {
                var stride = Math.ceil(powers.length / 10);
                var kept = [];
                for (var k = 0; k < powers.length; k += stride) kept.push(powers[k]);
                if (kept.length > 10) kept = kept.slice(0, 10);
                if (kept.length && kept[kept.length - 1] !== powers[powers.length - 1]) {
                    if (kept.length < 10) kept.push(powers[powers.length - 1]);
                    else kept[kept.length - 1] = powers[powers.length - 1];
                }
                powers = kept;
            }
            return powers;
        }
        var step = niceStep(max - min || 1, 6);
        var start = Math.ceil(min / step - 1e-9) * step;
        var out = [];
        for (var value = start; value <= max + step * 1e-9; value += step) {
            out.push(Math.round(value * 1e9) / 1e9);
        }
        if (!out.length) out = [min, max];
        return out;
    }

    function boxesOverlap(a, b) {
        return a.x < b.x + b.w && b.x < a.x + a.w &&
            a.y < b.y + b.h && b.y < a.y + a.h;
    }

    function layoutChart(spec) {
        var rows = spec.rows || [];
        var width = 640;
        var height = 320;
        var texts = [];
        var columns = rows.length ? Object.keys(rows[0]) : [];
        var xKey = columns[0] || "";
        var series = columns.slice(1);
        var type = spec.type === "bar" ? "bar" : "line";
        var xScale = spec.x && spec.x.scale === "log2" ? "log2" : "linear";
        var left = 58;
        var top = series.length > 1 ? 42 : 26;
        var bottom = 58;
        var right = 24;
        var maxEndLabel = 0;
        if (type === "line" && series.length > 1 && series.length <= 4) {
            for (var e = 0; e < series.length; e++) {
                maxEndLabel = Math.max(maxEndLabel, textWidth(series[e], 11, false) + 12);
            }
            right = Math.max(right, maxEndLabel);
        }
        var plotW = width - left - right;
        var plotH = height - top - bottom;
        var xValues = [];
        var i;
        for (i = 0; i < rows.length; i++) {
            xValues.push(type === "line" ? Number(rows[i][xKey]) : String(rows[i][xKey]));
        }
        var xMin = 0;
        var xMax = 1;
        var singleX = false;
        if (type === "line") {
            var nums = xValues.filter(function (v) { return Number.isFinite(v); });
            xMin = nums.length ? Math.min.apply(null, nums) : 0;
            xMax = nums.length ? Math.max.apply(null, nums) : 1;
            singleX = Math.abs(xMax - xMin) < 1e-12;
        }
        var logScale = xScale === "log2" && type === "line" && xMin > 0;
        var tx = function (value) { return logScale ? Math.log2(value) : value; };
        var txMin = tx(xMin);
        var txMax = tx(xMax);
        var yMin = spec.y && typeof spec.y.min === "number" ? spec.y.min : 0;
        var dataMax = -Infinity;
        for (i = 0; i < rows.length; i++) {
            for (var s = 0; s < series.length; s++) {
                var value = Number(rows[i][series[s]]);
                if (Number.isFinite(value)) dataMax = Math.max(dataMax, value);
            }
        }
        if (!Number.isFinite(dataMax)) dataMax = 1;
        var step = niceStep(dataMax - yMin || 1, 4);
        var yMax = Math.ceil((dataMax - yMin) / step) * step + yMin;
        if (yMax <= yMin) yMax = yMin + step;
        var ticks = [];
        for (var tv = yMin; tv <= yMax + step * 0.001; tv += step) ticks.push(tv);

        var xPosValue = function (value) {
            if (singleX || txMin === txMax) return left + plotW / 2;
            return left + (tx(value) - txMin) / (txMax - txMin) * plotW;
        };
        var xPos = function (index) {
            if (type === "bar") return left + (index + 0.5) * (plotW / Math.max(1, rows.length));
            return xPosValue(Number(xValues[index]));
        };
        var yPos = function (value) {
            return top + plotH - (value - yMin) / (yMax - yMin) * plotH;
        };

        for (i = 0; i < ticks.length; i++) {
            addText(texts, String(Math.round(ticks[i] * 1000) / 1000), left - 8, yPos(ticks[i]),
                11, left - 14, false, "end");
        }
        if (spec.x && spec.x.label) {
            addText(texts, spec.x.label, left + plotW / 2, height - 10, 12, plotW, false, "middle");
        }
        if (spec.y && spec.y.label) {
            var yLabel = spec.y.label + (spec.y.unit ? "（" + spec.y.unit + "）" : "");
            addText(texts, yLabel, 16, top + plotH / 2, 12, plotH, false, "middle");
            texts[texts.length - 1].rotated = true;
        }

        var legend = [];
        if (series.length > 1) {
            var lx = left;
            for (i = 0; i < series.length; i++) {
                var lw = textWidth(series[i], 11, false);
                legend.push({ label: series[i], index: i, x: lx, y: 16, w: lw + 18 });
                addText(texts, series[i], lx + 14, 22, 11, lw + 2, false, "start");
                lx += lw + 34;
            }
        }

        var bars = [];
        if (type === "bar") {
            var slot = plotW / Math.max(1, rows.length);
            var groupW = slot * 0.72;
            var gap = 2;
            var barW = (groupW - gap * (series.length - 1)) / Math.max(1, series.length);
            for (i = 0; i < rows.length; i++) {
                for (s = 0; s < series.length; s++) {
                    var v = Number(rows[i][series[s]]);
                    var yTop = yPos(v);
                    bars.push({
                        x: left + i * slot + (slot - groupW) / 2 + s * (barW + gap),
                        y: yTop, w: barW, h: top + plotH - yTop,
                        index: i, series: s, value: v
                    });
                }
            }
        }

        var points = [];
        if (type === "line") {
            for (i = 0; i < rows.length; i++) {
                for (s = 0; s < series.length; s++) {
                    var val = Number(rows[i][series[s]]);
                    points.push({ x: xPos(i), y: yPos(val), index: i, series: s, value: val });
                }
            }
        }

        var endLabels = [];
        if (type === "line" && series.length > 1 && series.length <= 4) {
            var endFont = 11;
            var endLineH = endFont * 1.22;
            var endCandidates = [];
            for (s = 0; s < series.length; s++) {
                var last = points[(rows.length - 1) * series.length + s];
                if (!last) continue;
                endCandidates.push({
                    text: series[s], x: last.x + 8, y: last.y, index: s,
                    maxWidth: right - 10,
                    w: textWidth(series[s], endFont, false), h: endLineH
                });
            }
            /* 从上往下放；重合时先往下挪一行，再往上挪一行，都不行就不画。
             * 图例不依赖这里，所以省掉末端标签不会丢信息。 */
            endCandidates.sort(function (a, b) {
                return a.y - b.y || a.index - b.index;
            });
            var placedEndBoxes = [];
            for (var ec = 0; ec < endCandidates.length; ec++) {
                var cand = endCandidates[ec];
                var endOffsets = [0, endLineH, -endLineH];
                var chosenY = null;
                for (var eo = 0; eo < endOffsets.length; eo++) {
                    var box = { x: cand.x, y: cand.y + endOffsets[eo] - cand.h / 2,
                                w: cand.w, h: cand.h };
                    if (box.y < 2 || box.y + box.h > height - 2) continue;
                    var clash = false;
                    for (var pb = 0; pb < placedEndBoxes.length; pb++) {
                        if (boxesOverlap(box, placedEndBoxes[pb])) {
                            clash = true;
                            break;
                        }
                    }
                    if (!clash) {
                        chosenY = box.y + box.h / 2;
                        placedEndBoxes.push(box);
                        break;
                    }
                }
                if (chosenY === null) continue;
                endLabels.push({
                    text: cand.text, x: cand.x, y: chosenY, index: cand.index,
                    maxWidth: cand.maxWidth, w: cand.w, h: cand.h
                });
            }
            endLabels.sort(function (a, b) { return a.index - b.index; });
            for (var el = 0; el < endLabels.length; el++) {
                addText(texts, endLabels[el].text, endLabels[el].x, endLabels[el].y,
                    endFont, endLabels[el].maxWidth, false, "start");
            }
        }

        /* x 轴刻度：折线图标数据里出现的 x 值（>10 个时取整齐刻度），
         * 柱状图每根柱子下面标类别，最多两行。 */
        var tickY = top + plotH + 15;
        var xTicks = [];
        if (type === "line") {
            var uniqueX = [];
            for (i = 0; i < xValues.length; i++) {
                var seen = false;
                for (var u = 0; u < uniqueX.length; u++) {
                    if (uniqueX[u] === xValues[i]) { seen = true; break; }
                }
                if (!seen) uniqueX.push(xValues[i]);
            }
            var tickValues = uniqueX.length <= 10 ? uniqueX :
                niceXTicks(xMin, xMax, logScale);
            var positions = tickValues.map(xPosValue);
            for (i = 0; i < tickValues.length; i++) {
                var gapW = plotW;
                if (i > 0) gapW = Math.min(gapW, positions[i] - positions[i - 1]);
                if (i < positions.length - 1) gapW = Math.min(gapW, positions[i + 1] - positions[i]);
                var maxW = Math.max(20, gapW - 4);
                var label = niceLabel(tickValues[i]);
                xTicks.push({
                    x: positions[i], y: tickY, lines: [label],
                    maxWidth: maxW, font: 10, value: tickValues[i]
                });
                addText(texts, label, positions[i], tickY, 10, maxW, false, "middle");
            }
        } else {
            var barSlot = plotW / Math.max(1, rows.length);
            for (i = 0; i < rows.length; i++) {
                var catWidth = Math.max(20, barSlot - 8);
                var lines = wrapChartLabel(String(xValues[i]), catWidth, 10, 2);
                var cx = left + (i + 0.5) * barSlot;
                xTicks.push({
                    x: cx, y: tickY, lines: lines, maxWidth: catWidth, font: 10, index: i
                });
                for (var li = 0; li < lines.length; li++) {
                    addText(texts, lines[li], cx, tickY + li * 12, 10, catWidth, false, "middle");
                }
            }
        }

        /* 单系列柱状图在柱顶写数值；多系列靠悬停和图例。 */
        var valueLabels = [];
        if (type === "bar" && series.length === 1) {
            for (i = 0; i < bars.length; i++) {
                var bar = bars[i];
                var valueText = niceLabel(bar.value);
                var valueY = Math.max(8, bar.y - 8);
                var valueW = Math.max(16, bar.w - 4);
                valueLabels.push({ x: bar.x + bar.w / 2, y: valueY, text: valueText, maxWidth: valueW });
                addText(texts, valueText, bar.x + bar.w / 2, valueY, 10, valueW, false, "middle");
            }
        }

        return {
            kind: "chart", width: width, height: height, type: type,
            left: left, top: top, plotW: plotW, plotH: plotH, bottom: bottom, right: right,
            rows: rows, xKey: xKey, series: series, xValues: xValues, xMin: xMin, xMax: xMax,
            xScale: xScale, logScale: logScale, xTicks: xTicks, valueLabels: valueLabels,
            yMin: yMin, yMax: yMax, ticks: ticks, step: step,
            xPos: xPos, yPos: yPos, points: points, bars: bars,
            legend: legend, endLabels: endLabels, note: spec.note || "", texts: texts
        };
    }

    /* ------------------------------------------------------------------ *
     * layout
     * ------------------------------------------------------------------ */

    function layout(spec) {
        if (!spec || !spec.kind) throw new Error("figure has no kind");
        if (spec.kind === "seq") return layoutSeq(spec);
        if (spec.kind === "grid") return layoutGrid(spec);
        if (spec.kind === "flow") return layoutFlow(spec);
        if (spec.kind === "chart") return layoutChart(spec);
        throw new Error("unknown figure kind: " + spec.kind);
    }

    /* ------------------------------------------------------------------ *
     * renderSVG
     * ------------------------------------------------------------------ */

    function renderSeq(spec, step, L) {
        var out = [svgOpen(L.width, L.height, spec)];
        var i;
        var r;
        var staticFig = !!spec.static || (spec.steps || []).length <= 1;
        /* 泳道头 */
        for (i = 0; i < L.lanes.length; i++) {
            var lane = L.lanes[i];
            var role = lane.role || "thread";
            out.push(rectSVG(L.left[i] + 4, 5, L.laneWidths[i] - 8, L.headerH - 12,
                "ccfig-lane-head ccfig-role-" + role));
            out.push(textSVG(L.centers[i], L.headerH / 2 - 1, lane.label || "",
                Math.max(10, L.labelFont), "ccfig-label ccfig-lane-title"));
        }

        for (r = 0; r <= Math.min(step, L.rows.length - 1); r++) {
            var row = L.rows[r];
            var isCurrent = !staticFig && r === step;
            if (isCurrent) {
                out.push(rectSVG(L.margin, row.top, L.width - L.margin * 2, row.height,
                    "ccfig-row-current"));
            }
            if (L.hasRowLabel && spec.steps[r] && spec.steps[r].label) {
                out.push(textSVG(L.margin + L.labelColW - 8, row.center,
                    spec.steps[r].label, L.labelFont, "ccfig-label ccfig-step-label", "end"));
            }
            /* 生命线 */
            for (i = 0; i < L.lanes.length; i++) {
                var id = L.lanes[i].id;
                if (r < L.startStep[id]) continue;
                var sb = row.statusBefore[id];
                var sa = row.statusAfter[id];
                if (sb === "done") continue;
                var y1 = row.top;
                var y2 = row.top + row.height;
                var status = sa === "new" ? "running" : sa;
                if (sa === "done") {
                    y2 = row.center;
                    status = sb;
                } else if (r === L.startStep[id]) {
                    y1 = row.center;
                }
                out.push('<line class="ccfig-lifeline ccfig-life-' + status + '" x1="' +
                    r2(L.centers[i]) + '" y1="' + r2(y1) + '" x2="' + r2(L.centers[i]) +
                    '" y2="' + r2(y2) + '"/>');
            }
            /* 操作 */
            for (var oi = 0; oi < row.ops.length; oi++) {
                var op = row.ops[oi];
                var ds = isCurrent ? ' data-step="' + r + '"' : "";
                if (op.type === "at") {
                    out.push('<g class="ccfig-op-group' + toneClass(op.tone) + '"' + ds + ">");
                    out.push(rectSVG(op.x, op.y, op.w, op.h, "ccfig-op", ' rx="4"'));
                    out.push(textSVG(op.x + op.w / 2, op.y + op.h / 2, op.text, op.font,
                        "ccfig-text ccfig-op-text"));
                    out.push("</g>");
                } else if (op.type === "span") {
                    out.push('<g class="ccfig-span-group' + toneClass(op.tone) + '"' + ds + ">");
                    out.push(rectSVG(op.x, op.y, op.w, op.h, "ccfig-span", ' rx="8"'));
                    out.push(textSVG(op.x + op.w / 2, op.y + op.h / 2, op.text, op.font,
                        "ccfig-span-text"));
                    out.push("</g>");
                } else if (op.type === "arrow") {
                    out.push('<g class="ccfig-arrow-group' + toneClass(op.tone) + '"' + ds + ">");
                    out.push('<line class="ccfig-arrow-line' + (op.dashed ? " ccfig-dashed" : "") +
                        '" x1="' + r2(op.x1) + '" y1="' + r2(op.y1) + '" x2="' + r2(op.x2) +
                        '" y2="' + r2(op.y2) + '"/>');
                    var dx = op.x2 - op.x1;
                    var dy = op.y2 - op.y1;
                    out.push(arrowHead(op.x2, op.y2, dx, dy, "ccfig-arrowhead"));
                    var lab = op.label;
                    out.push(rectSVG(lab.x, lab.y, lab.w, lab.h, "ccfig-arrow-label-bg", ' rx="3"'));
                    out.push(textSVG(lab.cx, lab.cy, lab.text, op.font, "ccfig-label ccfig-arrow-label"));
                    out.push("</g>");
                }
            }
        }

        /* state 面板 */
        if (L.stateCells.length) {
            var values = {};
            var changed = {};
            for (i = 0; i < L.state.length; i++) values[L.state[i].id] = L.state[i].value;
            for (r = 0; r <= Math.min(step, L.rows.length - 1); r++) {
                var set = L.rows[r] ? (spec.steps[r].set || {}) : {};
                for (var key in set) values[key] = set[key];
                if (r === step) {
                    for (var ck in set) changed[ck] = true;
                }
            }
            for (i = 0; i < L.stateCells.length; i++) {
                var sc = L.stateCells[i];
                var text = sc.label + " = " + (values[sc.id] === undefined ? "" : values[sc.id]);
                out.push('<g class="ccfig-state' + (changed[sc.id] ? " ccfig-flash" : "") + '">');
                out.push(rectSVG(sc.x, sc.y, sc.w, sc.h, "ccfig-state-cell", ' rx="3"'));
                out.push(textSVG(sc.x + sc.w / 2, sc.y + sc.h / 2, text, sc.font,
                    "ccfig-label ccfig-state-text"));
                out.push("</g>");
            }
        }

        out.push("</svg>");
        return out.join("");
    }

    function renderGrid(spec, step, L) {
        var out = [svgOpen(L.width, L.height, spec)];
        var i;
        /* 列号 */
        for (i = 0; i < L.colLabels.length; i++) {
            out.push(textSVG(L.colX(i) + L.cell / 2, L.top - 8,
                String(L.colLabels[i]), 10, "ccfig-label ccfig-axis-label"));
        }
        /* 行名 */
        for (i = 0; i < L.rows.length; i++) {
            out.push(textSVG(L.margin + L.rowLabelW - 8, L.rowY(i) + L.cell / 2,
                L.rows[i].label || "", 12, "ccfig-label ccfig-row-label", "end"));
        }
        /* brackets */
        for (i = 0; i < L.brackets.length; i++) {
            var br = L.brackets[i];
            if (br.below) {
                out.push('<path class="ccfig-bracket" d="M ' + r2(br.x1) + ' ' + r2(br.y - 6) +
                    ' L ' + r2(br.x1) + ' ' + r2(br.y) + ' L ' + r2(br.x2) + ' ' + r2(br.y) +
                    ' L ' + r2(br.x2) + ' ' + r2(br.y - 6) + '"/>');
                out.push(textSVG((br.x1 + br.x2) / 2, br.y + 18, br.label, 11,
                    "ccfig-label ccfig-bracket-label"));
            } else {
                out.push('<path class="ccfig-bracket" d="M ' + r2(br.x1) + ' ' + r2(br.y + 6) +
                    ' L ' + r2(br.x1) + ' ' + r2(br.y) + ' L ' + r2(br.x2) + ' ' + r2(br.y) +
                    ' L ' + r2(br.x2) + ' ' + r2(br.y + 6) + '"/>');
                out.push(textSVG((br.x1 + br.x2) / 2, br.y - 9, br.label, 11,
                    "ccfig-label ccfig-bracket-label"));
            }
        }

        /* 累积上色 */
        var fill = [];
        var text = [];
        for (var r = 0; r < L.rows.length; r++) {
            fill.push([]);
            text.push([]);
            for (var c = 0; c < L.cols; c++) {
                fill[r].push(null);
                text[r].push(L.cells[r][c].text);
            }
        }
        var marks = [];
        var changedCells = {};
        for (var s = 0; s <= Math.min(step, (spec.steps || []).length - 1); s++) {
            var st = spec.steps[s];
            var paints = st.paint || [];
            for (i = 0; i < paints.length; i++) {
                var p = paints[i];
                var rr = parseRange(p.r, L.rows.length, "row");
                var cc = parseRange(p.c, L.cols, "column");
                for (var y = rr[0]; y <= rr[1]; y++) {
                    for (var x = cc[0]; x <= cc[1]; x++) {
                        if (y < 0 || y >= L.rows.length || x < 0 || x >= L.cols) continue;
                        fill[y][x] = p.key === null || p.key === undefined ? null : p.key;
                        if (p.text !== undefined) text[y][x] = p.text;
                        if (s === step) changedCells[y + "," + x] = true;
                    }
                }
            }
            if (s === step) {
                var markList = st.mark || [];
                for (i = 0; i < markList.length; i++) {
                    var mk = markList[i];
                    var mr = parseRange(mk.r, L.rows.length, "row");
                    var mc = parseRange(mk.c, L.cols, "column");
                    for (var my = mr[0]; my <= mr[1]; my++) {
                        for (var mx = mc[0]; mx <= mc[1]; mx++) {
                            if (my < 0 || my >= L.rows.length || mx < 0 || mx >= L.cols) continue;
                            marks.push({ r: my, c: mx, tone: mk.tone || "hot" });
                        }
                    }
                }
            }
        }
        var legendIndex = {};
        for (i = 0; i < (spec.legend || []).length; i++) legendIndex[spec.legend[i].key] = i;

        for (r = 0; r < L.rows.length; r++) {
            for (c = 0; c < L.cols; c++) {
                var cell = L.cells[r][c];
                var key = fill[r][c];
                var cls = "ccfig-cell";
                var extra = "";
                if (key !== null && key !== undefined) {
                    var idx = legendIndex[key];
                    cls += " ccfig-filled " + colorClass(idx === undefined ? 0 : idx);
                }
                var cds = changedCells[r + "," + c] ? ' data-step="' + step + '"' : "";
                out.push(rectSVG(cell.x, cell.y, cell.w, cell.h, cls, cds));
                if (text[r][c] !== null && text[r][c] !== undefined && String(text[r][c]) !== "") {
                    var dark = key !== null && key !== undefined && legendIndex[key] === 3;
                    var onColor = key !== null && key !== undefined ? " ccfig-on-color" : "";
                    out.push(textSVG(cell.x + cell.w / 2, cell.y + cell.h / 2,
                        String(text[r][c]), cellFontFor(text[r][c], cell.w),
                        "ccfig-label ccfig-cell-text" + onColor +
                        (dark ? " ccfig-dark-text" : ""), "middle", true));
                }
            }
        }
        for (i = 0; i < marks.length; i++) {
            var m = marks[i];
            var rct = L.cells[m.r][m.c];
            out.push(rectSVG(rct.x + 1, rct.y + 1, rct.w - 2, rct.h - 2,
                "ccfig-mark" + toneClass(m.tone), ' rx="2" fill="none"'));
        }

        /* 图例 */
        for (i = 0; i < L.legend.length; i++) {
            var item = L.legend[i];
            out.push(rectSVG(item.x, item.y + 2, 12, 12, "ccfig-swatch " + colorClass(item.index), ' rx="2"'));
            out.push(textSVG(item.x + 16, item.y + 9, item.label, L.legendFont,
                "ccfig-label ccfig-legend-label", "start"));
        }
        out.push("</svg>");
        return out.join("");
    }

    function nodePath(node) {
        var pts = node.points.map(function (p, i) {
            return (i ? " L " : "M ") + r2(p.x) + " " + r2(p.y);
        }).join("");
        return pts;
    }

    function renderFlow(spec, step, L) {
        var out = [svgOpen(L.width, L.height, spec)];
        var i;
        /* 分区背景 */
        for (i = 0; i < L.bands.length; i++) {
            var band = L.bands[i];
            out.push(rectSVG(band.x, band.y, band.w, band.h,
                "ccfig-band ccfig-band-" + ((band.index % 3) + 1), ' rx="4"'));
            out.push(textSVG(band.x + 6, band.y + band.h / 2, band.label, L.bandFont,
                "ccfig-label ccfig-band-label", "start"));
        }

        /* 计算每一步的可见性、tone、sub、队列 */
        var visibleNodes = {};
        var visibleEdges = {};
        var tone = {};
        var subs = {};
        var queues = {};
        for (i = 0; i < L.nodes.length; i++) {
            visibleNodes[L.nodes[i].id] = !L.nodes[i].hidden;
            subs[L.nodes[i].id] = L.nodes[i].sub;
        }
        for (i = 0; i < L.edges.length; i++) visibleEdges[L.edges[i].id] = !L.edges[i].hidden;
        for (var s = 0; s <= Math.min(step, (spec.steps || []).length - 1); s++) {
            var st = spec.steps[s] || {};
            var show = st.show || [];
            var hide = st.hide || [];
            for (i = 0; i < show.length; i++) {
                visibleNodes[show[i]] = true;
                visibleEdges[show[i]] = true;
            }
            for (i = 0; i < hide.length; i++) {
                visibleNodes[hide[i]] = false;
                visibleEdges[hide[i]] = false;
            }
            if (st.tone) {
                for (var tid in st.tone) {
                    if (st.tone[tid] === null) delete tone[tid];
                    else tone[tid] = st.tone[tid];
                }
            }
            if (st.set) {
                for (var sid in st.set) subs[sid] = st.set[sid];
            }
            if (st.queue) {
                for (var qid in st.queue) queues[qid] = st.queue[qid];
            }
        }
        var current = spec.steps[Math.min(step, (spec.steps || []).length - 1)] || {};
        var hi = {};
        for (i = 0; i < (current.hi || []).length; i++) hi[current.hi[i]] = true;

        /* 边 */
        for (i = 0; i < L.edges.length; i++) {
            var edge = L.edges[i];
            if (!visibleEdges[edge.id]) continue;
            if (!visibleNodes[edge.from] || !visibleNodes[edge.to]) continue;
            var cls = "ccfig-edge" + (edge.dashed ? " ccfig-dashed" : "") + toneClass(tone[edge.id]) +
                (hi[edge.id] ? " ccfig-hi" : "");
            var ds = hi[edge.id] ? ' data-step="' + step + '"' : "";
            out.push('<g class="ccfig-edge-group' + toneClass(tone[edge.id]) + '"' + ds + ">");
            out.push('<path class="ccfig-edge-path ' + cls + '" data-edge="' + attr(edge.id) +
                '" d="' + nodePath(edge) + '"/>');
            if (edge.arrow === "end" || edge.arrow === "both") {
                var n = edge.points.length;
                out.push(arrowHead(edge.points[n - 1].x, edge.points[n - 1].y,
                    edge.points[n - 1].x - edge.points[n - 2].x,
                    edge.points[n - 1].y - edge.points[n - 2].y, "ccfig-arrowhead"));
            }
            if (edge.arrow === "both") {
                out.push(arrowHead(edge.points[0].x, edge.points[0].y,
                    edge.points[0].x - edge.points[1].x,
                    edge.points[0].y - edge.points[1].y, "ccfig-arrowhead"));
            }
            if (edge.label.text) {
                out.push(rectSVG(edge.label.x, edge.label.y, edge.label.w, edge.label.h,
                    "ccfig-edge-label-bg", ' rx="3"'));
                out.push(textSVG(edge.label.x + edge.label.w / 2, edge.label.y + edge.label.h / 2,
                    edge.label.text, L.labelFont, "ccfig-label ccfig-edge-label"));
            }
            out.push("</g>");
        }

        /* 节点 */
        for (i = 0; i < L.nodes.length; i++) {
            var node = L.nodes[i];
            if (!visibleNodes[node.id]) continue;
            var nodeCls = "ccfig-node-group" + toneClass(tone[node.id]) +
                (hi[node.id] ? " ccfig-hi" : "");
            var nds = hi[node.id] ? ' data-step="' + step + '"' : "";
            out.push('<g class="' + nodeCls + '"' + nds + ">");
            if (node.shape !== "note") {
                var shapeCls = "ccfig-node ccfig-shape-" + node.shape;
                var rx = node.shape === "round" ? 18 : (node.shape === "queue" ? 5 : 4);
                out.push(rectSVG(node.x, node.y, node.w, node.h, shapeCls, ' rx="' + rx + '"'));
                if (node.shape === "store") {
                    out.push('<line class="ccfig-store-line" x1="' + r2(node.x + 5) + '" y1="' +
                        r2(node.y + 8) + '" x2="' + r2(node.x + node.w - 5) + '" y2="' +
                        r2(node.y + 8) + '"/>');
                }
            }
            out.push(textSVG(node.x + node.w / 2, node.labelY, node.label, node.font,
                "ccfig-label ccfig-node-label"));
            if (subs[node.id]) {
                out.push(textSVG(node.x + node.w / 2, node.subY, subs[node.id],
                    node.subFont, "ccfig-label ccfig-node-sub"));
            }
            if (node.shape === "queue") {
                var q = queues[node.id] || [];
                var slots = node.slots;
                var slotW = (node.w - 12) / slots;
                for (var sl = 0; sl < slots; sl++) {
                    var sx = node.x + 6 + sl * slotW;
                    out.push(rectSVG(sx, node.slotY, slotW - 2, 20, "ccfig-slot", ' rx="2"'));
                    if (q[sl] !== undefined) {
                        out.push(textSVG(sx + (slotW - 2) / 2, node.slotY + 10, String(q[sl]), L.slotFont,
                            "ccfig-label ccfig-slot-text"));
                    }
                }
            }
            out.push("</g>");
        }

        /* 当前步的 token */
        var tokens = current.token || [];
        for (i = 0; i < tokens.length; i++) {
            var token = tokens[i];
            var e = L.edgesById[token.edge];
            if (!e) continue;
            var tw = textWidth(token.label || "", L.labelFont, false) + 10;
            var tx = e.token.x;
            var ty = e.token.y;
            out.push('<g class="ccfig-token" data-edge="' + attr(token.edge) + '" data-x="' +
                r2(tx) + '" data-y="' + r2(ty) + '" transform="translate(0,0)">');
            out.push(rectSVG(tx - tw / 2, ty - 8, tw, 16, "ccfig-token-bg", ' rx="8"'));
            out.push(textSVG(tx, ty, token.label || "", L.labelFont, "ccfig-label ccfig-token-text"));
            out.push("</g>");
        }
        out.push("</svg>");
        return out.join("");
    }

    function niceLabel(value) {
        return String(Math.round(value * 1000) / 1000);
    }

    function roundedBarPath(x, y, w, h) {
        var radius = Math.min(4, w / 2, h);
        return "M " + r2(x) + " " + r2(y + radius) +
            " Q " + r2(x) + " " + r2(y) + " " + r2(x + radius) + " " + r2(y) +
            " L " + r2(x + w - radius) + " " + r2(y) +
            " Q " + r2(x + w) + " " + r2(y) + " " + r2(x + w) + " " + r2(y + radius) +
            " L " + r2(x + w) + " " + r2(y + h) +
            " L " + r2(x) + " " + r2(y + h) + " Z";
    }

    function renderChart(spec, step, L) {
        var out = [svgOpen(L.width, L.height, spec)];
        var i;
        /* 网格 */
        for (i = 0; i < L.ticks.length; i++) {
            var ty = L.yPos(L.ticks[i]);
            out.push('<line class="ccfig-grid-line" x1="' + r2(L.left) + '" y1="' + r2(ty) +
                '" x2="' + r2(L.left + L.plotW) + '" y2="' + r2(ty) + '"/>');
            out.push(textSVG(L.left - 8, ty, niceLabel(L.ticks[i]), 11,
                "ccfig-label ccfig-tick-label", "end"));
        }
        out.push('<line class="ccfig-axis" x1="' + r2(L.left) + '" y1="' + r2(L.top) +
            '" x2="' + r2(L.left) + '" y2="' + r2(L.top + L.plotH) + '"/>');
        out.push('<line class="ccfig-axis" x1="' + r2(L.left) + '" y1="' + r2(L.top + L.plotH) +
            '" x2="' + r2(L.left + L.plotW) + '" y2="' + r2(L.top + L.plotH) + '"/>');

        /* x 轴刻度值 / 柱状图类别名 */
        for (i = 0; i < L.xTicks.length; i++) {
            var xt = L.xTicks[i];
            out.push('<line class="ccfig-axis-tick" x1="' + r2(xt.x) + '" y1="' +
                r2(L.top + L.plotH) + '" x2="' + r2(xt.x) + '" y2="' +
                r2(L.top + L.plotH + 4) + '"/>');
            for (var xl = 0; xl < xt.lines.length; xl++) {
                out.push(textSVG(xt.x, xt.y + xl * 12, xt.lines[xl], 10,
                    "ccfig-label ccfig-tick-label"));
            }
        }

        if (spec.x && spec.x.label) {
            out.push(textSVG(L.left + L.plotW / 2, L.height - 10, spec.x.label, 12,
                "ccfig-label ccfig-axis-title"));
        }
        if (spec.y && spec.y.label) {
            var yLabel = spec.y.label + (spec.y.unit ? "（" + spec.y.unit + "）" : "");
            out.push('<g transform="translate(16,' + r2(L.top + L.plotH / 2) + ') rotate(-90)">' +
                textSVG(0, 0, yLabel, 12, "ccfig-label ccfig-axis-title") + "</g>");
        }

        /* 图例 */
        for (i = 0; i < L.legend.length; i++) {
            var lg = L.legend[i];
            out.push(rectSVG(lg.x, lg.y, 10, 10, "ccfig-swatch " + colorClass(lg.index), ' rx="2"'));
            out.push(textSVG(lg.x + 14, lg.y + 6, lg.label, 11,
                "ccfig-label ccfig-legend-label", "start"));
        }

        if (L.type === "bar") {
            for (i = 0; i < L.bars.length; i++) {
                var bar = L.bars[i];
                out.push('<path class="ccfig-bar ' + colorClass(bar.series) + '" d="' +
                    roundedBarPath(bar.x, bar.y, bar.w, Math.max(0, bar.h)) + '"/>');
            }
            for (i = 0; i < L.valueLabels.length; i++) {
                var vl = L.valueLabels[i];
                out.push(textSVG(vl.x, vl.y, vl.text, 10,
                    "ccfig-label ccfig-value-label"));
            }
            for (i = 0; i < L.bars.length; i++) {
                var b = L.bars[i];
                out.push('<rect class="ccfig-hit" x="' + r2(b.x - 2) + '" y="' + r2(L.top) +
                    '" width="' + r2(b.w + 4) + '" height="' + r2(L.plotH) +
                    '" data-bar="' + i + '"/>');
            }
        } else {
            for (i = 0; i < L.series.length; i++) {
                var pts = [];
                for (var j = 0; j < L.rows.length; j++) {
                    var p = L.points[j * L.series.length + i];
                    pts.push(r2(p.x) + "," + r2(p.y));
                }
                out.push('<polyline class="ccfig-line ' + colorClass(i) + '" points="' +
                    pts.join(" ") + '"/>');
                for (j = 0; j < L.rows.length; j++) {
                    var pt = L.points[j * L.series.length + i];
                    out.push('<circle class="ccfig-point ' + colorClass(i) + '" cx="' + r2(pt.x) +
                        '" cy="' + r2(pt.y) + '" r="4"/>');
                }
            }
            for (i = 0; i < L.rows.length; i++) {
                var slot = L.plotW / Math.max(1, L.rows.length);
                var hx = L.rows.length <= 1 ? L.left + L.plotW / 2 - slot / 2 : L.xPos(i) - slot / 2;
                out.push('<rect class="ccfig-hit" x="' + r2(hx) + '" y="' + r2(L.top) +
                    '" width="' + r2(slot) + '" height="' + r2(L.plotH) + '" data-x="' + i + '"/>');
            }
            for (i = 0; i < L.endLabels.length; i++) {
                var el = L.endLabels[i];
                out.push(textSVG(el.x, el.y, el.text, 11,
                    "ccfig-label ccfig-end-label " + colorClass(el.index), "start"));
            }
            out.push('<line class="ccfig-guide" x1="0" y1="' + r2(L.top) + '" x2="0" y2="' +
                r2(L.top + L.plotH) + '"/>');
        }
        out.push('<g class="ccfig-tooltip"><rect class="ccfig-tooltip-bg" x="0" y="0" width="1" height="1" rx="4"/>' +
            '<text class="ccfig-label ccfig-tooltip-text" x="0" y="0"></text></g>');
        out.push("</svg>");
        return out.join("");
    }

    function renderSVG(spec, step) {
        var L = layout(spec);
        var last = spec.kind === "chart" ? 0 : Math.max(0, (spec.steps || []).length - 1);
        var s = clamp(Math.trunc(step === undefined ? 0 : step), 0, last);
        if (spec.kind === "seq") return renderSeq(spec, s, L);
        if (spec.kind === "grid") return renderGrid(spec, s, L);
        if (spec.kind === "flow") return renderFlow(spec, s, L);
        return renderChart(spec, s, L);
    }

    if (typeof module !== "undefined" && module.exports) {
        module.exports = {
            textWidth: textWidth,
            layout: layout,
            renderSVG: renderSVG
        };
    }

    if (typeof document === "undefined") return;

    /* ------------------------------------------------------------------ *
     * 浏览器部分
     * ------------------------------------------------------------------ */

    var REDUCED = typeof window !== "undefined" && typeof window.matchMedia === "function" &&
        window.matchMedia("(prefers-reduced-motion: reduce)").matches;

    var ICONS = {
        replay: '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M12 5V2L7 6l5 4V7a5 5 0 1 1-5 5H5a7 7 0 1 0 7-7z"/></svg>',
        prev: '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M6 5h2v14H6zM19 5l-10 7 10 7z"/></svg>',
        play: '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M7 4l13 8-13 8z"/></svg>',
        pause: '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M6 4h4v16H6zM14 4h4v16h-4z"/></svg>',
        next: '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M16 5h2v14h-2zM5 5l10 7-10 7z"/></svg>'
    };

    var figures = {};
    var api = { setStep: setFigureStep, getStep: getFigureStep, figures: figures };
    window.ccfig = api;

    function setFigureStep(id, step) {
        if (figures[id] && figures[id].setStep) figures[id].setStep(step);
    }

    function getFigureStep(id) {
        return figures[id] ? figures[id].step : 0;
    }

    function isStatic(spec) {
        if (spec.kind === "chart") return true;
        return !!spec.static || (spec.steps || []).length <= 1;
    }

    function initFigure(fig) {
        var stage = fig.querySelector(".ccfig-stage");
        var script = fig.querySelector(".ccfig-spec");
        if (!stage || !script) return;
        var spec;
        try {
            spec = JSON.parse(script.textContent);
        } catch (err) {
            stage.textContent = "图的数据读不出来：" + err.message;
            return;
        }
        var steps = spec.kind === "chart" ? [] : (spec.steps || []);
        var staticFig = isStatic(spec);
        var last = spec.kind === "chart" ? 0 : Math.max(0, steps.length - 1);
        var state = {
            spec: spec,
            step: staticFig ? last : 0,
            playing: false,
            autoplay: !staticFig && !REDUCED,
            visible: false,
            timer: null,
            hold: null,
            setStep: null
        };

        var controls = null;
        var counter = null;
        var note = null;
        var playButton = null;

        if (!staticFig) {
            controls = document.createElement("div");
            controls.className = "ccfig-controls";
            controls.setAttribute("role", "group");
            controls.setAttribute("aria-label", "图播放控制");
            controls.innerHTML =
                '<button type="button" class="ccfig-btn" data-act="replay" aria-label="重播" title="重播">' +
                ICONS.replay + '</button>' +
                '<button type="button" class="ccfig-btn" data-act="prev" aria-label="上一步" title="上一步">' +
                ICONS.prev + '</button>' +
                '<button type="button" class="ccfig-btn" data-act="play" aria-label="播放" title="播放">' +
                ICONS.play + '</button>' +
                '<button type="button" class="ccfig-btn" data-act="next" aria-label="下一步" title="下一步">' +
                ICONS.next + '</button>' +
                '<span class="ccfig-counter" aria-label="步数"></span>';
            stage.insertAdjacentElement("afterend", controls);
            counter = controls.querySelector(".ccfig-counter");
            playButton = controls.querySelector('[data-act="play"]');
            note = document.createElement("div");
            note.className = "ccfig-note";
            note.setAttribute("aria-live", "polite");
            controls.insertAdjacentElement("afterend", note);
        }

        function draw() {
            stage.innerHTML = renderSVG(spec, state.step);
            if (!REDUCED) {
                var fresh = stage.querySelectorAll('[data-step="' + state.step + '"]');
                for (var i = 0; i < fresh.length; i++) fresh[i].classList.add("ccfig-fade");
            }
            if (counter) counter.textContent = (state.step + 1) + " / " + steps.length;
            if (note) note.textContent = steps[state.step] ? (steps[state.step].note || "") : "";
            if (playButton) {
                var playing = state.playing;
                playButton.setAttribute("aria-label", playing ? "暂停" : "播放");
                playButton.setAttribute("title", playing ? "暂停" : "播放");
                playButton.innerHTML = playing ? ICONS.pause : ICONS.play;
            }
            if (!staticFig && !REDUCED) animateTokens(state.step);
            if (spec.kind === "chart") {
                var chartNote = fig.querySelector(".ccfig-chart-note");
                if (spec.note && !chartNote) {
                    chartNote = document.createElement("div");
                    chartNote.className = "ccfig-chart-note";
                    chartNote.textContent = spec.note;
                    stage.insertAdjacentElement("afterend", chartNote);
                }
                bindChart();
            }
        }

        function animateTokens(step) {
            var svg = stage.querySelector("svg");
            if (!svg) return;
            var tokens = svg.querySelectorAll(".ccfig-token");
            for (var i = 0; i < tokens.length; i++) {
                (function (token) {
                    var edge = token.getAttribute("data-edge");
                    var path = svg.querySelector('.ccfig-edge-path[data-edge="' + cssEscape(edge) + '"]');
                    if (!path || typeof path.getTotalLength !== "function") return;
                    var total = path.getTotalLength();
                    if (!total) return;
                    var baseX = parseFloat(token.getAttribute("data-x")) || 0;
                    var baseY = parseFloat(token.getAttribute("data-y")) || 0;
                    var started = null;
                    function frame(now) {
                        if (started === null) started = now;
                        var t = Math.min(1, (now - started) / 600);
                        var point = path.getPointAtLength(total * t);
                        token.setAttribute("transform", "translate(" +
                            (point.x - baseX) + "," + (point.y - baseY) + ")");
                        if (t < 1) requestAnimationFrame(frame);
                        else token.classList.add("ccfig-token-out");
                    }
                    requestAnimationFrame(frame);
                })(tokens[i]);
            }
        }

        function cssEscape(value) {
            if (window.CSS && CSS.escape) return CSS.escape(value);
            return String(value).replace(/["\\]/g, "\\$&");
        }

        function clearTimers() {
            if (state.timer) clearTimeout(state.timer);
            if (state.hold) clearTimeout(state.hold);
            state.timer = null;
            state.hold = null;
        }

        function schedule() {
            clearTimers();
            if (!state.playing) return;
            var speed = Number(spec.speed) > 0 ? Number(spec.speed) : 1400;
            state.timer = setTimeout(function () {
                if (!state.playing) return;
                if (state.step < last) {
                    state.step += 1;
                    draw();
                    schedule();
                } else {
                    state.hold = setTimeout(function () {
                        if (!state.playing) return;
                        state.step = 0;
                        draw();
                        schedule();
                    }, 3000);
                }
            }, speed);
        }

        function setStep(value) {
            state.step = clamp(Math.trunc(value), 0, last);
            draw();
        }

        function userTookOver(stopPlayback) {
            state.autoplay = false;
            if (state.observer) {
                state.observer.disconnect();
                state.observer = null;
            }
            if (stopPlayback) {
                state.playing = false;
                clearTimers();
                draw();
            }
        }

        if (controls) {
            controls.addEventListener("click", function (event) {
                var button = event.target.closest("button[data-act]");
                if (!button) return;
                var act = button.getAttribute("data-act");
                if (act === "replay") {
                    userTookOver(true);
                    state.step = 0;
                    state.playing = true;
                    schedule();
                    draw();
                } else if (act === "prev") {
                    userTookOver(true);
                    setStep(state.step - 1);
                } else if (act === "next") {
                    userTookOver(true);
                    setStep(state.step + 1);
                } else if (act === "play") {
                    userTookOver(false);
                    state.playing = !state.playing;
                    if (state.playing) schedule(); else clearTimers();
                    draw();
                }
            });
        }

        fig.addEventListener("keydown", function (event) {
            var tag = event.target && event.target.tagName;
            if (tag === "INPUT" || tag === "TEXTAREA" || tag === "SELECT" || tag === "BUTTON") return;
            if (event.key === "ArrowLeft") {
                userTookOver(true);
                setStep(state.step - 1);
                event.preventDefault();
            } else if (event.key === "ArrowRight") {
                userTookOver(true);
                setStep(state.step + 1);
                event.preventDefault();
            } else if (event.key === " " || event.key === "Spacebar") {
                userTookOver(false);
                state.playing = !state.playing;
                if (state.playing) schedule(); else clearTimers();
                draw();
                event.preventDefault();
            } else if (event.key === "Home") {
                userTookOver(true);
                state.step = 0;
                draw();
                event.preventDefault();
            }
        });

        if (staticFig) {
            draw();
        } else {
            draw();
            if (!REDUCED && "IntersectionObserver" in window) {
                state.observer = new IntersectionObserver(function (entries) {
                    var entry = entries[0];
                    state.visible = entry.intersectionRatio >= 0.4;
                    if (state.visible && state.autoplay && !state.playing) {
                        state.playing = true;
                        schedule();
                        draw();
                    } else if (!state.visible && state.autoplay && state.playing) {
                        state.playing = false;
                        clearTimers();
                        draw();
                    }
                }, { threshold: [0, 0.4, 1] });
                state.observer.observe(fig);
            }
            window.addEventListener("beforeprint", function () {
                if (staticFig) return;
                state.printStep = state.step;
                state.step = last;
                draw();
            });
            window.addEventListener("afterprint", function () {
                if (staticFig || state.printStep === undefined) return;
                state.step = state.printStep;
                draw();
            });
        }

        state.setStep = setStep;
        figures[fig.id] = state;
        fig._ccfig = state;

        function bindChart() {
            var svg = stage.querySelector("svg");
            if (!svg) return;
            var L = layout(spec);
            var tooltip = svg.querySelector(".ccfig-tooltip");
            var guide = svg.querySelector(".ccfig-guide");
            if (!tooltip) return;

            function setTooltip(lines, x, y) {
                var text = tooltip.querySelector(".ccfig-tooltip-text");
                var box = tooltip.querySelector(".ccfig-tooltip-bg");
                while (text.firstChild) text.removeChild(text.firstChild);
                var width = 0;
                for (var i = 0; i < lines.length; i++) {
                    var span = document.createElementNS("http://www.w3.org/2000/svg", "tspan");
                    span.setAttribute("x", "6");
                    span.setAttribute("y", String(8 + i * 15));
                    span.textContent = lines[i];
                    text.appendChild(span);
                    width = Math.max(width, textWidth(lines[i], 11, false));
                }
                var h = 10 + lines.length * 15;
                box.setAttribute("width", String(width + 12));
                box.setAttribute("height", String(h));
                text.setAttribute("x", "0");
                text.setAttribute("y", "0");
                var tx = Math.min(L.width - width - 16, x + 12);
                var ty = Math.max(4, y - h - 4);
                tooltip.setAttribute("transform", "translate(" + tx + "," + ty + ")");
                tooltip.classList.add("visible");
            }

            function hide() {
                tooltip.classList.remove("visible");
                if (guide) guide.classList.remove("visible");
            }

            svg.addEventListener("mouseleave", hide);
            var hits = svg.querySelectorAll(".ccfig-hit");
            for (var i = 0; i < hits.length; i++) {
                (function (hit) {
                    hit.addEventListener("mouseenter", function () {
                        if (L.type === "bar") {
                            var bar = L.bars[parseInt(hit.getAttribute("data-bar"), 10)];
                            if (!bar) return;
                            setTooltip([
                                String(L.xValues[bar.index]),
                                L.series[bar.series] + " = " + niceLabel(bar.value)
                            ], bar.x + bar.w / 2, bar.y);
                        } else {
                            var index = parseInt(hit.getAttribute("data-x"), 10);
                            var px = L.xPos(index);
                            if (guide) {
                                guide.setAttribute("x1", String(px));
                                guide.setAttribute("x2", String(px));
                                guide.classList.add("visible");
                            }
                            var lines = [String(L.xValues[index])];
                            for (var s = 0; s < L.series.length; s++) {
                                var point = L.points[index * L.series.length + s];
                                lines.push(L.series[s] + " = " + niceLabel(point.value));
                            }
                            setTooltip(lines, px, L.top + 8);
                        }
                    });
                })(hits[i]);
            }
        }
    }

    function init() {
        var nodes = document.querySelectorAll("figure.ccfig");
        for (var i = 0; i < nodes.length; i++) initFigure(nodes[i]);

        /* 打印时展开所有“文字版”，打印完恢复原来的开合状态。 */
        var printOpen = [];
        window.addEventListener("beforeprint", function () {
            printOpen = [];
            var details = document.querySelectorAll("details.ccfig-text");
            for (var j = 0; j < details.length; j++) {
                printOpen.push(details[j].open);
                details[j].open = true;
            }
        });
        window.addEventListener("afterprint", function () {
            var details = document.querySelectorAll("details.ccfig-text");
            for (var j = 0; j < details.length && j < printOpen.length; j++) {
                details[j].open = printOpen[j];
            }
        });
    }

    if (document.readyState === "loading") {
        document.addEventListener("DOMContentLoaded", init);
    } else {
        init();
    }
})();
