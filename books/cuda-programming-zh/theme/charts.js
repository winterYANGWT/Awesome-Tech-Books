/*
 * 第 1 章的两张统计图：柱状图（1.2）和折线图（1.3）。
 *
 * 数据不在脚本里，而是从 figure[data-source] 指向的 <details> 的 <code>
 * 文本表里解析。图中的颜色全部来自 charts.css，脚本只负责加 class。
 */
(function () {
  "use strict";

  var SVG_NS = "http://www.w3.org/2000/svg";
  var NUMERIC_CELL = /^(-?\d+(?:\.\d+)?)(?:\s*(\S+))?$/;
  var SERIES_CLASSES = ["cu-s1", "cu-s2"];

  /* ------------------------------------------------------------------ *
   * 纯函数（Node 测试会直接 require 这些）
   * ------------------------------------------------------------------ */

  function parseNumericCell(cell) {
    var match = NUMERIC_CELL.exec(cell);
    if (!match) return null;
    return {
      value: Number(match[1]),
      text: match[1],
      unit: match[2] || undefined
    };
  }

  function parseColumns(text) {
    if (typeof text !== "string") {
      throw new Error("表格内容必须是字符串");
    }

    var sourceLines = text.split(/\r?\n/);
    var lines = [];
    for (var i = 0; i < sourceLines.length; i++) {
      var line = sourceLines[i].replace(/\s+$/, "");
      if (!line.trim()) continue;
      lines.push(line);
    }
    if (!lines.length) throw new Error("表格为空");

    var notes = [];
    var header = null;
    var rows = [];
    var valueCount = 0;
    var dataStarted = false;

    for (var n = 0; n < lines.length; n++) {
      var cells = lines[n].trim().split(/\s{2,}/);
      if (cells.length >= 2) {
        var parsed = [];
        var allNumeric = true;
        for (var c = 1; c < cells.length; c++) {
          var cell = parseNumericCell(cells[c]);
          if (!cell) {
            allNumeric = false;
            break;
          }
          parsed.push(cell);
        }

        if (allNumeric) {
          if (!dataStarted) {
            dataStarted = true;
            valueCount = parsed.length;
          }
          if (parsed.length !== valueCount) {
            throw new Error("每行的值列数不一致");
          }
          rows.push({ label: cells[0], values: parsed });
        } else if (!dataStarted) {
          header = cells;
        } else {
          throw new Error("数据行之后出现了无法解析的行：" + lines[n]);
        }
      } else {
        notes.push(cells[0]);
      }
    }

    if (!rows.length) throw new Error("表格里没有数据行");

    var labelName = null;
    var columnNames = [];
    if (header) {
      if (header.length === valueCount + 1) {
        labelName = header[0];
        columnNames = header.slice(1);
      } else if (header.length === valueCount) {
        columnNames = header.slice();
      } else {
        throw new Error("表头列数与数据列数不一致");
      }
    } else {
      for (var k = 0; k < valueCount; k++) {
        columnNames.push("列 " + (k + 1));
      }
    }

    var columns = columnNames.map(function (name, index) {
      var unit;
      for (var r = 0; r < rows.length; r++) {
        if (rows[r].values[index].unit) {
          unit = rows[r].values[index].unit;
          break;
        }
      }
      return { name: name, unit: unit };
    });

    return {
      labelName: labelName,
      columns: columns,
      rows: rows,
      notes: notes
    };
  }

  function niceTicks(max, approxCount) {
    max = Number(max);
    approxCount = Number(approxCount);
    if (!isFinite(approxCount) || approxCount <= 0) approxCount = 4;
    if (!isFinite(max) || max <= 0) return [0];

    var rawStep = max / approxCount;
    var exponent = Math.floor(Math.log10(rawStep));
    var magnitude = Math.pow(10, exponent);
    var fraction = rawStep / magnitude;
    var niceFraction;

    if (fraction <= 1) niceFraction = 1;
    else if (fraction <= 2) niceFraction = 2;
    else if (fraction <= 2.5) niceFraction = 2.5;
    else if (fraction <= 5) niceFraction = 5;
    else niceFraction = 10;

    var step = niceFraction * magnitude;
    var last = Math.ceil(max / step - 1e-12) * step;
    if (last < max - 1e-9) last += step;

    var count = Math.round(last / step);
    var ticks = [];
    for (var i = 0; i <= count; i++) {
      ticks.push(Number((i * step).toPrecision(12)));
    }
    while (ticks[ticks.length - 1] < max - 1e-9) {
      ticks.push(Number((ticks.length * step).toPrecision(12)));
    }
    return ticks;
  }

  function trimUnitValue(value) {
    var rounded = Math.round(value * 10) / 10;
    return String(rounded);
  }

  function formatThreads(n) {
    var value = Number(n);
    if (!isFinite(value)) return String(n);
    if (Math.abs(value) >= 1024 * 1024) {
      return trimUnitValue(value / (1024 * 1024)) + "M";
    }
    if (Math.abs(value) >= 1024) {
      return trimUnitValue(value / 1024) + "K";
    }
    return String(value);
  }

  function formatSpeedup(x) {
    var value = Number(x);
    if (!isFinite(value)) return "—";
    if (Math.abs(value) >= 100) return String(Math.round(value)) + "×";
    if (Math.abs(value) >= 10) return value.toFixed(1) + "×";
    return value.toFixed(2) + "×";
  }

  function nearestIndex(xs, x, log) {
    if (!xs || !xs.length) return -1;
    var best = 0;
    var bestDistance = Infinity;
    for (var i = 0; i < xs.length; i++) {
      var distance;
      if (log && xs[i] > 0 && x > 0) {
        distance = Math.abs(Math.log(xs[i]) - Math.log(x));
      } else {
        distance = Math.abs(xs[i] - x);
      }
      if (distance < bestDistance) {
        bestDistance = distance;
        best = i;
      }
    }
    return best;
  }

  if (typeof module === "object" && module.exports) {
    module.exports = {
      parseColumns: parseColumns,
      niceTicks: niceTicks,
      formatThreads: formatThreads,
      formatSpeedup: formatSpeedup,
      nearestIndex: nearestIndex
    };
  }

  /* ------------------------------------------------------------------ *
   * 浏览器里的渲染
   * ------------------------------------------------------------------ */

  if (typeof document === "undefined") return;

  var chartStates = [];

  function boot() {
    var figures = document.querySelectorAll(".cu-chart");
    if (!figures.length) return;

    for (var i = 0; i < figures.length; i++) {
      setupChart(figures[i]);
    }

    document.addEventListener("pointerdown", function (event) {
      var target = event.target;
      if (target && target.closest && target.closest(".cu-chart")) return;
      hideAll();
    });

    document.addEventListener("keydown", function (event) {
      if (event.key === "Escape") hideAll();
    });
  }

  function hideAll() {
    for (var i = 0; i < chartStates.length; i++) {
      if (typeof chartStates[i].hide === "function") chartStates[i].hide();
    }
  }

  function setupChart(fig) {
    var plot = fig.querySelector(".cu-chart-plot");
    var script = fig.querySelector('script[type="application/json"]');
    var sourceId = fig.getAttribute("data-source");
    var source = sourceId ? document.getElementById(sourceId) : null;

    try {
      if (!plot) throw new Error("缺少 .cu-chart-plot");
      if (!script) throw new Error("缺少 application/json 配置");
      if (!source) throw new Error("找不到 data-source 指向的元素");

      var code = source.querySelector("code");
      if (!code) throw new Error("data-source 下没有 code 元素");

      var config = JSON.parse(script.textContent || "");
      if (!config || (config.kind !== "bars" && config.kind !== "line")) {
        throw new Error("kind 必须是 bars 或 line");
      }

      var data = parseColumns(code.textContent || "");

      if (!fig.querySelector(".cu-chart-head")) {
        var tip = document.createElement("div");
        tip.className = "cu-chart-tip";
        tip.setAttribute("aria-hidden", "true");

        var live = document.createElement("div");
        live.className = "cu-sr-only";
        live.setAttribute("aria-live", "polite");
        live.setAttribute("aria-atomic", "true");

        fig.appendChild(tip);
        fig.appendChild(live);

        var state = {
          fig: fig,
          plot: plot,
          config: config,
          data: data,
          tip: tip,
          live: live,
          tipVisible: false,
          width: 0,
          hide: function () {}
        };

        if (config.kind === "bars") initBars(state);
        else initLine(state);

        state.render();
        chartStates.push(state);
        watchSize(state);
      }
    } catch (error) {
      console.warn("[cu-chart] 图表渲染失败：", error);
      fig.classList.add("cu-chart-failed");
      if (source && "open" in source) source.open = true;
    }
  }

  function watchSize(state) {
    if (typeof ResizeObserver !== "function") return;

    var frame = 0;
    var observer = new ResizeObserver(function () {
      if (frame) return;
      frame = requestAnimationFrame(function () {
        frame = 0;
        var width = Math.round(state.plot.clientWidth);
        if (width < 1) return;
        if (Math.abs(width - state.width) >= 1) state.render(width);
      });
    });
    observer.observe(state.plot);
  }

  /* ---------------------------- 通用小工具 ---------------------------- */

  function element(tag, className, text) {
    var node = document.createElement(tag);
    if (className) node.className = className;
    if (text !== undefined) node.textContent = text;
    return node;
  }

  function svgElement(tag, className) {
    var node = document.createElementNS(SVG_NS, tag);
    if (className) node.setAttribute("class", className);
    return node;
  }

  function addText(parent, x, y, text, className, anchor) {
    var node = svgElement("text", className);
    node.setAttribute("x", String(round(x)));
    node.setAttribute("y", String(round(y)));
    if (anchor) node.setAttribute("text-anchor", anchor);
    node.textContent = text;
    parent.appendChild(node);
    return node;
  }

  function addLine(parent, x1, y1, x2, y2, className) {
    var node = svgElement("line", className);
    node.setAttribute("x1", String(round(x1)));
    node.setAttribute("y1", String(round(y1)));
    node.setAttribute("x2", String(round(x2)));
    node.setAttribute("y2", String(round(y2)));
    parent.appendChild(node);
    return node;
  }

  function round(value) {
    return Math.round(value * 100) / 100;
  }

  function measureWidth(plot, fig) {
    var width = plot.clientWidth;
    if (!width) width = fig.clientWidth;
    if (!width) width = 600;
    return Math.max(1, Math.round(width));
  }

  var canvasContext = null;

  function textWidth(family, size, weight, text) {
    if (!canvasContext) {
      var canvas = document.createElement("canvas");
      canvasContext = canvas.getContext("2d");
    }
    if (!canvasContext) return String(text).length * size * 0.62;
    canvasContext.font = (weight || 400) + " " + size + "px " + (family || "sans-serif");
    return canvasContext.measureText(String(text)).width;
  }

  function hoverHint() {
    try {
      if (window.matchMedia && window.matchMedia("(hover: hover)").matches) {
        return "鼠标悬停查看数值";
      }
      return "点按查看数值";
    } catch (error) {
      return "鼠标悬停查看数值";
    }
  }

  function makeHead(fig, plot, left, right) {
    var head = element("div", "cu-chart-head");
    var leftBox = element("div", "cu-chart-head-left");
    var rightBox = element("div", "cu-chart-head-right");
    leftBox.appendChild(left);
    rightBox.appendChild(right);
    head.appendChild(leftBox);
    head.appendChild(rightBox);
    fig.insertBefore(head, plot);
    return head;
  }

  function captionText(fig) {
    var caption = fig.querySelector("figcaption");
    return caption ? caption.textContent.trim() : "";
  }

  function figurePoint(fig, clientX, clientY) {
    var rect = fig.getBoundingClientRect();
    return { x: clientX - rect.left, y: clientY - rect.top };
  }

  function plotPoint(fig, plot, x, y) {
    var figureRect = fig.getBoundingClientRect();
    var plotRect = plot.getBoundingClientRect();
    return {
      x: plotRect.left - figureRect.left + x,
      y: plotRect.top - figureRect.top + y
    };
  }

  function showTip(state, lines, x, y) {
    var tip = state.tip;
    tip.textContent = "";

    var spoken = [];
    for (var i = 0; i < lines.length; i++) {
      var item = lines[i];
      var row = element("div", "cu-tip-line " + (item.className || ""));
      if (item.keyClass) {
        var key = element("span", "cu-tip-key " + item.keyClass);
        row.appendChild(key);
      }
      row.appendChild(element("span", "cu-tip-text", item.text));
      tip.appendChild(row);
      spoken.push(item.text);
    }

    tip.classList.add("cu-show");

    var figureRect = state.fig.getBoundingClientRect();
    var figureWidth = figureRect.width || state.width || 600;
    var tipWidth = tip.offsetWidth;
    var tipHeight = tip.offsetHeight;

    var left = x - tipWidth / 2;
    if (left < 6) left = 6;
    if (left + tipWidth > figureWidth - 6) {
      left = Math.max(6, figureWidth - tipWidth - 6);
    }

    var top = y - tipHeight - 12;
    if (top < 4) top = y + 18;

    tip.style.left = Math.round(left) + "px";
    tip.style.top = Math.round(top) + "px";
    state.live.textContent = spoken.join("；");
    state.tipVisible = true;
  }

  function hideTip(state) {
    if (!state.tip) return;
    state.tip.classList.remove("cu-show");
    state.live.textContent = "";
    state.tipVisible = false;
  }

  function formatTick(value) {
    if (Number.isInteger(value)) return String(value);
    return String(Number(value.toFixed(4)));
  }

  function formatThousands(value) {
    return String(Math.round(value)).replace(/\B(?=(\d{3})+(?!\d))/g, ",");
  }

  function groupName(label) {
    return label.split(/[\s，,]/)[0];
  }

  function findSpread(config, row, column) {
    var spreads = config.spread || [];
    for (var i = 0; i < spreads.length; i++) {
      if (spreads[i].row === row && spreads[i].col === column) return spreads[i];
    }
    return null;
  }

  /* ------------------------------ 柱状图 ------------------------------ */

  function initBars(state) {
    var fig = state.fig;
    var plot = state.plot;
    var config = state.config;
    var data = state.data;

    var groupClasses = {};
    var groups = [];
    for (var i = 0; i < data.rows.length; i++) {
      var group = groupName(data.rows[i].label);
      if (!Object.prototype.hasOwnProperty.call(groupClasses, group)) {
        groupClasses[group] = SERIES_CLASSES[groups.length] || "cu-s1";
        groups.push(group);
      }
    }

    var legend = element("div", "cu-chart-legend");
    for (var g = 0; g < groups.length; g++) {
      var item = element("span", "cu-chart-legend-item");
      var swatch = element("span", "cu-chart-legend-swatch " + groupClasses[groups[g]]);
      item.appendChild(swatch);
      item.appendChild(element("span", "", groups[g]));
      legend.appendChild(item);
    }

    var right = element("div", "cu-chart-right-controls");
    right.appendChild(element("span", "cu-chart-hint", hoverHint()));
    makeHead(fig, plot, legend, right);

    var hotBar = null;
    var hotRow = null;

    function clearHot() {
      if (hotBar) hotBar.classList.remove("cu-hot");
      if (hotRow) hotRow.classList.remove("cu-hot");
      hotBar = null;
      hotRow = null;
    }

    state.hide = function () {
      clearHot();
      hideTip(state);
    };

    state.render = function (requestedWidth) {
      var width = requestedWidth || measureWidth(plot, fig);
      state.width = width;
      hideTip(state);
      clearHot();
      while (plot.firstChild) plot.removeChild(plot.firstChild);

      var family = getComputedStyle(fig).fontFamily || "sans-serif";
      var labelFont = 13;
      var valueFont = 12;
      var tickFont = 12;
      var rowHeight = 30;
      var barHeight = 18;
      var titleHeight = 26;
      var tickHeight = 24;
      var gap = 16;
      var topPadding = 4;

      var labelWidth = 0;
      for (var r = 0; r < data.rows.length; r++) {
        labelWidth = Math.max(
          labelWidth,
          textWidth(family, labelFont, 400, data.rows[r].label)
        );
      }
      labelWidth = Math.ceil(labelWidth) + 4;

      var tickSets = [];
      var axisMax = [];
      var valueWidth = 0;
      var lastTickWidth = 0;

      for (var c = 0; c < data.columns.length; c++) {
        var maxValue = 0;
        for (var rowIndex = 0; rowIndex < data.rows.length; rowIndex++) {
          var valueCell = data.rows[rowIndex].values[c];
          if (valueCell) maxValue = Math.max(maxValue, valueCell.value);
          var spread = findSpread(config, data.rows[rowIndex].label, data.columns[c].name);
          if (spread) maxValue = Math.max(maxValue, Number(spread.lo), Number(spread.hi));
        }
        tickSets[c] = niceTicks(maxValue > 0 ? maxValue : 1, 4);
        axisMax[c] = tickSets[c][tickSets[c].length - 1] || 1;

        var columnUnit = config.unit || data.columns[c].unit || "";
        for (var v = 0; v < data.rows.length; v++) {
          var cell = data.rows[v].values[c];
          if (!cell) continue;
          valueWidth = Math.max(
            valueWidth,
            textWidth(family, valueFont, 400, cell.text + " " + columnUnit)
          );
        }
        lastTickWidth = Math.max(
          lastTickWidth,
          textWidth(
            family,
            tickFont,
            400,
            formatTick(axisMax[c]) + (columnUnit ? " " + columnUnit : "")
          )
        );
      }

      var barsLeft = labelWidth + 12;
      var rightPadding = Math.ceil(
        Math.max(valueWidth + 12, lastTickWidth / 2 + 10, 16)
      );
      var plotWidth = Math.max(60, width - barsLeft - rightPadding);
      var svgWidth = barsLeft + plotWidth + rightPadding;

      var barsHeight = data.rows.length * rowHeight;
      var panelHeight = titleHeight + barsHeight + tickHeight;
      var svgHeight =
        topPadding +
        data.columns.length * panelHeight +
        Math.max(0, data.columns.length - 1) * gap +
        8;

      var svg = svgElement("svg");
      svg.setAttribute("width", String(svgWidth));
      svg.setAttribute("height", String(svgHeight));
      svg.setAttribute("viewBox", "0 0 " + svgWidth + " " + svgHeight);
      svg.setAttribute("role", "group");
      svg.setAttribute("aria-label", captionText(fig));

      for (var colIndex = 0; colIndex < data.columns.length; colIndex++) {
        var column = data.columns[colIndex];
        var columnUnit = config.unit || column.unit || "";
        var panelTop = topPadding + colIndex * (panelHeight + gap);
        var barsTop = panelTop + titleHeight;
        var barsBottom = barsTop + barsHeight;
        var max = axisMax[colIndex];
        var ticks = tickSets[colIndex];

        addText(svg, 0, panelTop + 15, column.name, "cu-panel-name");
        var nameWidth = textWidth(family, labelFont, 600, column.name);
        var description = config.panels && config.panels[column.name];
        if (description) {
          addText(
            svg,
            nameWidth + 8,
            panelTop + 15,
            description,
            "cu-panel-sub"
          );
        }

        for (var t = 0; t < ticks.length; t++) {
          var tickX = barsLeft + (ticks[t] / max) * plotWidth;
          if (ticks[t] > 0) {
            addLine(svg, tickX, barsTop, tickX, barsBottom, "cu-grid");
          }
          var isLast = t === ticks.length - 1;
          var tickLabel =
            formatTick(ticks[t]) + (isLast && columnUnit ? " " + columnUnit : "");
          addText(svg, tickX, barsBottom + 17, tickLabel, "cu-tick", "middle");
        }
        addLine(
          svg,
          barsLeft,
          barsBottom,
          barsLeft + plotWidth,
          barsBottom,
          "cu-axis"
        );

        for (var rowIndex2 = 0; rowIndex2 < data.rows.length; rowIndex2++) {
          var row = data.rows[rowIndex2];
          var cell2 = row.values[colIndex];
          if (!cell2) continue;

          var rowTop = barsTop + rowIndex2 * rowHeight;
          var centerY = rowTop + rowHeight / 2;
          var spread2 = findSpread(config, row.label, column.name);

          addText(
            svg,
            labelWidth,
            centerY + 4,
            row.label,
            "cu-label",
            "end"
          );

          var barLength = Math.max(2, (cell2.value / max) * plotWidth);
          var barEnd = barsLeft + barLength;
          var top = centerY - barHeight / 2;
          var bottom = centerY + barHeight / 2;
          var barClass =
            "cu-bar " + (groupClasses[groupName(row.label)] || "cu-s1");
          var bar = svgElement("path", barClass);
          bar.setAttribute("d", barPath(barsLeft, barEnd, top, bottom, 4));
          svg.appendChild(bar);

          var spreadEnd = 0;
          if (spread2) {
            var lowX = barsLeft + (Number(spread2.lo) / max) * plotWidth;
            var highX = barsLeft + (Number(spread2.hi) / max) * plotWidth;
            spreadEnd = highX;
            var spreadPath = svgElement("path", "cu-spread");
            spreadPath.setAttribute(
              "d",
              "M " +
                round(lowX) +
                " " +
                round(centerY) +
                " H " +
                round(highX) +
                " M " +
                round(lowX) +
                " " +
                round(centerY - 3) +
                " V " +
                round(centerY + 3) +
                " M " +
                round(highX) +
                " " +
                round(centerY - 3) +
                " V " +
                round(centerY + 3)
            );
            svg.appendChild(spreadPath);
          }

          var valueX = Math.max(barEnd, spreadEnd) + 6;
          addText(
            svg,
            valueX,
            centerY + 4,
            cell2.text + (columnUnit ? " " + columnUnit : ""),
            "cu-value",
            "start"
          );

          addBarHit({
            svg: svg,
            row: row,
            column: column,
            columnIndex: colIndex,
            cell: cell2,
            unit: columnUnit,
            config: config,
            bar: bar,
            x: barsLeft,
            barEnd: barEnd,
            valueEnd: valueX,
            centerY: centerY,
            rowTop: rowTop,
            rowHeight: rowHeight,
            svgWidth: svgWidth,
            groupClass: groupClasses[groupName(row.label)] || "cu-s1",
            spread: spread2
          });
        }
      }

      plot.appendChild(svg);

      function addBarHit(info) {
        var hit = svgElement("rect", "cu-hit");
        hit.setAttribute("x", "0");
        hit.setAttribute("y", String(round(info.rowTop)));
        hit.setAttribute("width", String(round(info.svgWidth)));
        hit.setAttribute("height", String(info.rowHeight));
        hit.setAttribute("tabindex", "0");

        var valueText =
          info.cell.text + (info.unit ? " " + info.unit : "");
        var line1 = {
          keyClass: info.groupClass,
          text: info.row.label + " · " + info.column.name
        };
        var line2 = { className: "cu-tip-value", text: valueText };
        var lines = [line1, line2];
        var aria = info.row.label + " · " + info.column.name + "，" + valueText;

        if (config.baseline) {
          if (info.row.label === config.baseline) {
            lines.push({ className: "cu-tip-meta", text: "基准" });
            aria += "，基准";
          } else {
            var baselineRow = findRow(config.baseline);
            var baselineCell = baselineRow && baselineRow.values[info.columnIndex];
            if (baselineCell && info.cell.value !== 0) {
              var speedup =
                config.better === "higher"
                  ? info.cell.value / baselineCell.value
                  : baselineCell.value / info.cell.value;
              var speedupText = formatSpeedup(speedup);
              lines.push({
                className: "cu-tip-meta",
                text:
                  "相对 " +
                  config.baseline +
                  "的加速比 " +
                  speedupText
              });
              aria += "，相对 " + config.baseline + "的加速比 " + speedupText;
            }
          }
        }

        if (info.spread && info.spread.note) {
          lines.push({ className: "cu-tip-note", text: info.spread.note });
          aria += "，" + info.spread.note;
        }

        hit.setAttribute("aria-label", aria);

        function showPointer(event) {
          var point = figurePoint(fig, event.clientX, event.clientY);
          showRow(point.x, point.y);
        }

        function showRow(x, y) {
          clearHot();
          hotBar = info.bar;
          hotRow = hit;
          info.bar.classList.add("cu-hot");
          hit.classList.add("cu-hot");
          showTip(state, lines, x, y);
        }

        function showAtBar() {
          var point = plotPoint(
            fig,
            plot,
            Math.max(info.barEnd, info.valueEnd) + 4,
            info.centerY
          );
          showRow(point.x, point.y);
        }

        function clearRow() {
          clearHot();
          hideTip(state);
        }

        hit.addEventListener("pointerenter", function (event) {
          if (event.pointerType === "touch") return;
          showPointer(event);
        });
        hit.addEventListener("pointermove", showPointer);
        hit.addEventListener("pointerleave", function () {
          if (document.activeElement !== hit) clearRow();
        });
        hit.addEventListener("pointerdown", showPointer);
        hit.addEventListener("focus", function () {
          showAtBar();
        });
        hit.addEventListener("blur", function () {
          clearRow();
        });
        hit.addEventListener("keydown", function (event) {
          if (event.key === "Escape") clearRow();
        });

        svg.appendChild(hit);
      }

      function findRow(label) {
        for (var i = 0; i < data.rows.length; i++) {
          if (data.rows[i].label === label) return data.rows[i];
        }
        return null;
      }
    };
  }

  function barPath(x0, x1, top, bottom, radius) {
    var width = x1 - x0;
    var r = Math.min(radius, width, (bottom - top) / 2);
    if (width <= 0.01) return "";
    if (r < 0.5) {
      return (
        "M " +
        round(x0) +
        " " +
        round(top) +
        " H " +
        round(x1) +
        " V " +
        round(bottom) +
        " H " +
        round(x0) +
        " Z"
      );
    }
    return (
      "M " +
      round(x0) +
      " " +
      round(top) +
      " H " +
      round(x1 - r) +
      " Q " +
      round(x1) +
      " " +
      round(top) +
      " " +
      round(x1) +
      " " +
      round(top + r) +
      " V " +
      round(bottom - r) +
      " Q " +
      round(x1) +
      " " +
      round(bottom) +
      " " +
      round(x1 - r) +
      " " +
      round(bottom) +
      " H " +
      round(x0) +
      " Z"
    );
  }

  /* ------------------------------ 折线图 ------------------------------ */

  function initLine(state) {
    var fig = state.fig;
    var plot = state.plot;
    var config = state.config;
    var data = state.data;
    var unit = (data.columns[0] && data.columns[0].unit) || "";

    if (!data.rows.length || !data.columns.length) {
      throw new Error("折线图没有数据");
    }

    var yLabel = config.yLabel || data.columns[0].name;
    var left = element("span", "cu-chart-ylabel", yLabel);
    var right = element("div", "cu-chart-right-controls");
    right.appendChild(element("span", "cu-chart-hint", hoverHint()));

    var toggle = null;
    var linearButton = null;
    var logButton = null;

    if (config.logToggle) {
      toggle = element("div", "cu-chart-toggle");
      toggle.setAttribute("role", "group");
      toggle.setAttribute("aria-label", "纵轴刻度");
      toggle.appendChild(element("span", "cu-chart-toggle-label", "纵轴"));

      linearButton = element("button", "cu-chart-toggle-button", "线性");
      linearButton.setAttribute("type", "button");
      logButton = element("button", "cu-chart-toggle-button", "对数");
      logButton.setAttribute("type", "button");
      toggle.appendChild(linearButton);
      toggle.appendChild(logButton);
      right.appendChild(toggle);
    }

    makeHead(fig, plot, left, right);

    state.log = false;
    state.selectedIndex = null;
    state.active = false;

    var activeElements = [];

    function clearActive() {
      for (var i = 0; i < activeElements.length; i++) {
        var node = activeElements[i];
        if (node.parentNode) node.parentNode.removeChild(node);
      }
      activeElements = [];
    }

    function hideLine() {
      state.active = false;
      hideTip(state);
      clearActive();
    }

    state.hide = hideLine;

    function updateButtons() {
      if (!linearButton || !logButton) return;
      linearButton.setAttribute(
        "aria-pressed",
        state.log ? "false" : "true"
      );
      logButton.setAttribute("aria-pressed", state.log ? "true" : "false");
    }

    function setLog(value) {
      state.log = value;
      updateButtons();
      state.render(state.width);
    }

    if (linearButton && logButton) {
      linearButton.addEventListener("click", function () {
        setLog(false);
      });
      logButton.addEventListener("click", function () {
        setLog(true);
      });
    }

    updateButtons();

    state.render = function (requestedWidth) {
      var width = requestedWidth || measureWidth(plot, fig);
      var restoreTip = state.tipVisible && state.selectedIndex !== null;
      state.width = width;
      clearActive();
      hideTip(state);
      while (plot.firstChild) plot.removeChild(plot.firstChild);

      var family = getComputedStyle(fig).fontFamily || "sans-serif";
      var marginLeft = 46;
      var marginRight = 18;
      var marginTop = 16;
      var plotHeight = 280;
      var marginBottom = 58;
      var plotWidth = Math.max(40, width - marginLeft - marginRight);
      var svgWidth = marginLeft + plotWidth + marginRight;
      var svgHeight = marginTop + plotHeight + marginBottom;

      var xs = [];
      var ys = [];
      var values = [];
      for (var i = 0; i < data.rows.length; i++) {
        var x = Number(data.rows[i].label);
        if (!isFinite(x)) throw new Error("行标签不是数字：" + data.rows[i].label);
        xs.push(x);
        var cell = data.rows[i].values[0];
        ys.push(cell);
        values.push(cell.value);
      }

      var xMin = Math.min.apply(null, xs);
      var xMax = Math.max.apply(null, xs);
      var dataMin = Math.min.apply(null, values);
      var dataMax = Math.max.apply(null, values);
      var refs = config.refs || [];
      var refMax = 0;
      for (var refIndex = 0; refIndex < refs.length; refIndex++) {
        refMax = Math.max(refMax, Number(refs[refIndex].y));
      }

      var yMin;
      var yMax;
      var yTicks;

      if (state.log) {
        var minPower = Math.floor(Math.log10(dataMin));
        var maxPower = Math.ceil(Math.log10(Math.max(dataMax, refMax)));
        yMin = Math.pow(10, minPower);
        yMax = Math.pow(10, maxPower);
        yTicks = [];
        for (var power = minPower; power <= maxPower; power++) {
          yTicks.push(Number(Math.pow(10, power).toPrecision(12)));
        }
      } else {
        var domainMax =
          config.yMax !== undefined && config.yMax !== null
            ? Number(config.yMax)
            : Math.max(dataMax, refMax);
        yTicks = niceTicks(domainMax, 4);
        yMin = 0;
        yMax = yTicks[yTicks.length - 1];
        if (!yMax) {
          yTicks = [0, 1];
          yMax = 1;
        }
      }

      var plotLeftInner = marginLeft + 8;
      var plotRightInner = marginLeft + plotWidth - 8;

      function sx(value) {
        if (xMax === xMin) return marginLeft + plotWidth / 2;
        var ratio =
          (Math.log(value) - Math.log(xMin)) /
          (Math.log(xMax) - Math.log(xMin));
        return plotLeftInner + ratio * (plotRightInner - plotLeftInner);
      }

      function sy(value) {
        if (state.log) {
          var ratio =
            (Math.log10(value) - Math.log10(yMin)) /
            (Math.log10(yMax) - Math.log10(yMin));
          return marginTop + plotHeight - ratio * plotHeight;
        }
        return marginTop + plotHeight - (value / yMax) * plotHeight;
      }

      var svg = svgElement("svg");
      svg.setAttribute("width", String(svgWidth));
      svg.setAttribute("height", String(svgHeight));
      svg.setAttribute("viewBox", "0 0 " + svgWidth + " " + svgHeight);
      svg.setAttribute("role", "group");
      svg.setAttribute("aria-label", captionText(fig));

      var firstPower = Math.ceil(Math.log2(xMin));
      var lastPower = Math.floor(Math.log2(xMax));
      var labelEvery = plotWidth < 420 ? 3 : 2;
      var labelIndex = 0;

      for (var power2 = firstPower; power2 <= lastPower; power2++) {
        var gridValue = Math.pow(2, power2);
        var gridX = sx(gridValue);
        addLine(
          svg,
          gridX,
          marginTop,
          gridX,
          marginTop + plotHeight,
          "cu-grid"
        );
        if (labelIndex % labelEvery === 0) {
          addText(
            svg,
            gridX,
            marginTop + plotHeight + 17,
            formatThreads(gridValue),
            "cu-tick",
            "middle"
          );
        }
        labelIndex++;
      }

      for (var yt = 0; yt < yTicks.length; yt++) {
        var tickY = sy(yTicks[yt]);
        if (yTicks[yt] !== 0) {
          addLine(
            svg,
            marginLeft,
            tickY,
            marginLeft + plotWidth,
            tickY,
            "cu-grid"
          );
        }
        addText(
          svg,
          marginLeft - 8,
          tickY + 4,
          formatTick(yTicks[yt]),
          "cu-tick",
          "end"
        );
      }

      addLine(
        svg,
        marginLeft,
        marginTop,
        marginLeft,
        marginTop + plotHeight,
        "cu-axis"
      );
      addLine(
        svg,
        marginLeft,
        marginTop + plotHeight,
        marginLeft + plotWidth,
        marginTop + plotHeight,
        "cu-axis"
      );

      var refLabels = [];
      for (var ref = 0; ref < refs.length; ref++) {
        var refY = sy(Number(refs[ref].y));
        if (refY < marginTop - 0.5 || refY > marginTop + plotHeight + 0.5) {
          continue;
        }
        addLine(
          svg,
          marginLeft,
          refY,
          marginLeft + plotWidth,
          refY,
          "cu-ref"
        );
        refLabels.push(
          addText(
            svg,
            marginLeft + 2,
            refY - 6,
            refs[ref].label,
            "cu-ref-label",
            "start"
          )
        );
      }

      var pathData = "";
      for (var pointIndex = 0; pointIndex < xs.length; pointIndex++) {
        pathData +=
          (pointIndex ? " L " : "M ") +
          round(sx(xs[pointIndex])) +
          " " +
          round(sy(values[pointIndex]));
      }
      var line = svgElement("path", "cu-line cu-s1");
      line.setAttribute("d", pathData);
      line.setAttribute("fill", "none");
      svg.appendChild(line);

      for (var dotIndex = 0; dotIndex < xs.length; dotIndex++) {
        var dot = svgElement("circle", "cu-dot cu-s1");
        dot.setAttribute("cx", String(round(sx(xs[dotIndex]))));
        dot.setAttribute("cy", String(round(sy(values[dotIndex]))));
        dot.setAttribute("r", "4");
        svg.appendChild(dot);
      }

      // 参考线的文字挪到折线之上，靠描边和折线隔开（窄屏时折线会穿过文字）。
      for (var refLabel = 0; refLabel < refLabels.length; refLabel++) {
        if (refLabels[refLabel]) svg.appendChild(refLabels[refLabel]);
      }

      if (xs.length) {
        addText(
          svg,
          sx(xs[0]) + 6,
          sy(values[0]) + (state.log ? 18 : -9),
          ys[0].text + (unit ? " " + unit : ""),
          "cu-point-label",
          "start"
        );
        addText(
          svg,
          sx(xs[xs.length - 1]) - 6,
          sy(values[values.length - 1]) + (state.log ? 20 : -9),
          ys[ys.length - 1].text + (unit ? " " + unit : ""),
          "cu-point-label",
          "end"
        );
      }

      addText(
        svg,
        marginLeft + plotWidth / 2,
        marginTop + plotHeight + 45,
        config.xLabel || data.labelName || "",
        "cu-axis-name",
        "middle"
      );

      var overlay = svgElement("rect", "cu-hit cu-line-hit");
      overlay.setAttribute("x", String(marginLeft));
      overlay.setAttribute("y", String(marginTop));
      overlay.setAttribute("width", String(plotWidth));
      overlay.setAttribute("height", String(plotHeight));
      overlay.setAttribute("tabindex", "0");
      overlay.setAttribute(
        "aria-label",
        "折线图：方向键逐点查看，Home 和 End 跳到两端"
      );
      svg.appendChild(overlay);

      plot.appendChild(svg);

      function pointPosition(index) {
        return plotPoint(
          fig,
          plot,
          sx(xs[index]),
          sy(values[index])
        );
      }

      function addActive(index) {
        clearActive();
        var x = sx(xs[index]);
        var y = sy(values[index]);
        var crosshair = svgElement("line", "cu-crosshair");
        crosshair.setAttribute("x1", String(round(x)));
        crosshair.setAttribute("x2", String(round(x)));
        crosshair.setAttribute("y1", String(marginTop));
        crosshair.setAttribute("y2", String(marginTop + plotHeight));
        svg.appendChild(crosshair);

        var activeDot = svgElement("circle", "cu-dot cu-active cu-s1");
        activeDot.setAttribute("cx", String(round(x)));
        activeDot.setAttribute("cy", String(round(y)));
        activeDot.setAttribute("r", "6");
        svg.appendChild(activeDot);

        activeElements = [crosshair, activeDot];
      }

      function tooltipLines(index) {
        var lines = [
          {
            keyClass: "cu-s1",
            text:
              (data.labelName || "x") +
              " " +
              formatThousands(xs[index])
          },
          {
            className: "cu-tip-value",
            text: ys[index].text + (unit ? " " + unit : "")
          }
        ];
        if (config.share && config.share.of) {
          var ratio = (values[index] / Number(config.share.of)) * 100;
          // 不到 10% 时保留一位小数，免得 0.7 GB/s 显示成“0%”。
          var percent = ratio < 10 ? ratio.toFixed(1) : String(Math.round(ratio));
          lines.push({
            className: "cu-tip-meta",
            text: (config.share.label || "占比") + "的 " + percent + "%"
          });
        }
        return lines;
      }

      function showIndex(index, x, y) {
        state.selectedIndex = index;
        state.active = true;
        addActive(index);
        showTip(state, tooltipLines(index), x, y);
      }

      function showIndexAtPoint(index) {
        var point = pointPosition(index);
        showIndex(index, point.x, point.y);
      }

      function hideButKeepSelection() {
        state.active = false;
        hideTip(state);
        clearActive();
      }

      function nearestFromPointer(event) {
        var rect = overlay.getBoundingClientRect();
        var px = event.clientX - rect.left;
        if (px < marginLeft) px = marginLeft;
        if (px > marginLeft + plotWidth) px = marginLeft + plotWidth;
        var ratio = (px - plotLeftInner) / (plotRightInner - plotLeftInner);
        if (ratio < 0) ratio = 0;
        if (ratio > 1) ratio = 1;
        var value;
        if (xMax === xMin) {
          value = xMin;
        } else {
          value = Math.exp(
            Math.log(xMin) + ratio * (Math.log(xMax) - Math.log(xMin))
          );
        }
        return nearestIndex(xs, value, true);
      }

      overlay.addEventListener("pointermove", function (event) {
        var index = nearestFromPointer(event);
        var point = figurePoint(fig, event.clientX, event.clientY);
        showIndex(index, point.x, point.y);
      });

      overlay.addEventListener("pointerdown", function (event) {
        var index = nearestFromPointer(event);
        var point = figurePoint(fig, event.clientX, event.clientY);
        showIndex(index, point.x, point.y);
      });

      overlay.addEventListener("pointerleave", function () {
        hideTip(state);
      });

      overlay.addEventListener("focus", function () {
        var index =
          state.selectedIndex === null
            ? xs.length - 1
            : state.selectedIndex;
        showIndexAtPoint(index);
      });

      overlay.addEventListener("blur", function () {
        hideButKeepSelection();
      });

      overlay.addEventListener("keydown", function (event) {
        var index = state.selectedIndex;
        if (event.key === "ArrowRight" || event.key === "ArrowLeft") {
          // mdBook 用左右方向键翻章，焦点在图上时不能让它收到。
          event.preventDefault();
          event.stopPropagation();
          if (index === null) index = event.key === "ArrowRight" ? 0 : xs.length - 1;
          else index += event.key === "ArrowRight" ? 1 : -1;
          if (index < 0) index = 0;
          if (index > xs.length - 1) index = xs.length - 1;
          showIndexAtPoint(index);
        } else if (event.key === "Home" || event.key === "End") {
          event.preventDefault();
          event.stopPropagation();
          showIndexAtPoint(event.key === "Home" ? 0 : xs.length - 1);
        } else if (event.key === "Escape") {
          hideButKeepSelection();
        }
      });

      if (state.active && state.selectedIndex !== null) {
        addActive(state.selectedIndex);
        if (restoreTip) {
          var restored = pointPosition(state.selectedIndex);
          showTip(
            state,
            tooltipLines(state.selectedIndex),
            restored.x,
            restored.y
          );
        }
      }
    };

    state.render();
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", boot);
  } else {
    boot();
  }
})();
