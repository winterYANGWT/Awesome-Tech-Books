/*
 * CUDA 书全书的统计图引擎：柱状图（bars）、折线图（line）、天平图（balance）、
 * roofline 图。
 *
 * 数据不在脚本里，而是从 figure[data-source] 指向的元素里的程序输出原文解析：
 * 可以是页面上不显示的 <script type="text/plain">，也可以是含 <code> 的元素。
 * 配置只写 JSON；图中的颜色全部来自 charts.css，脚本只负责加 class。
 *
 * 纯函数（parseColumns / parseLead / parseLaps / pickRows / extractTable /
 * validateChart / parseXValue / formatXValue / byteLinearTicks /
 * rooflineLimit / findLabelOffset 等）挂在 module.exports 上，Node 测试和
 * 作者检查脚本直接用。
 */
(function () {
  "use strict";

  var SVG_NS = "http://www.w3.org/2000/svg";
  var SERIES_CLASSES = ["cu-s1", "cu-s2", "cu-s3", "cu-s4"];
  var NUMBER_BODY =
    "-?(?:\\d{1,3}(?:,\\d{3})+|\\d+)(?:\\.\\d+)?(?:[eE][-+]?\\d+)?";
  var NUMERIC_CELL = new RegExp("^(" + NUMBER_BODY + ")(?:\\s*(\\S+))?$");
  var VALUE_CELL = new RegExp("^(" + NUMBER_BODY + ")(?:\\s*(\\S+))?$");
  var LEAD_UNIT = /^(?:[A-Za-zµ%]+(?:\/[A-Za-z]+)?|亿次\/秒|次\/秒|万|倍)$/;
  var PICK_NUMBER =
    /-?\d[\d,]*(?:\.\d+)?(?:e[-+]?\d+)?\s*(?:[A-Za-z%µ]+(?:\/[A-Za-z]+)?|亿次\/秒|万|倍)?/g;
  var MISSING_TEXT = "没测";

  /* ------------------------------------------------------------------ *
   * 纯函数（Node 测试会直接 require 这些）
   * ------------------------------------------------------------------ */

  function makeCell(numberText, unit) {
    return {
      value: Number(String(numberText).replace(/,/g, "")),
      text: String(numberText),
      unit: unit || undefined
    };
  }

  function parseNumericCell(cell) {
    var match = NUMERIC_CELL.exec(cell);
    if (!match) return null;
    return makeCell(match[1], match[2]);
  }

  function parsePickedToken(token) {
    var match = VALUE_CELL.exec(String(token).trim());
    if (!match) throw new Error("无法解析数值：" + token);
    return makeCell(match[1], match[2]);
  }

  // pick 的 cell 写成 null 表示这一行这一列没测；解析结果里就是 null。
  function isMissingCell(cell) {
    return cell === null || cell === undefined;
  }

  // 按缺值把一条线切成若干段，每段都是连续有值的点。缺值本身不进入任何段。
  function splitLineSegments(points) {
    var segments = [];
    var current = [];
    for (var i = 0; i < points.length; i++) {
      if (isMissingCell(points[i] && points[i].cell)) {
        if (current.length) {
          segments.push(current);
          current = [];
        }
      } else {
        current.push(points[i]);
      }
    }
    if (current.length) segments.push(current);
    return segments;
  }

  // tooltip 里的数值文本；缺值写“没测”，不能当成 0。
  function pointValueText(point, unit) {
    if (!point || isMissingCell(point.cell)) return MISSING_TEXT;
    var pointUnit = point.cell.unit || unit;
    return point.cell.text + (pointUnit ? " " + pointUnit : "");
  }

  function stripGroupTitle(text) {
    return String(text).trim().replace(/[：:]\s*$/, "");
  }

  function defaultColumnName(index) {
    return "值 " + (index + 1);
  }

  function inferColumns(rows, names, autoFlags) {
    return names.map(function (name, index) {
      var unit;
      for (var r = 0; r < rows.length; r++) {
        if (rows[r].values[index] && rows[r].values[index].unit) {
          unit = rows[r].values[index].unit;
          break;
        }
      }
      return {
        name: name,
        unit: unit,
        auto: !!(autoFlags && autoFlags[index])
      };
    });
  }

  function finishGroups(blocks, notes, fallbackNames, fallbackIsAuto) {
    var groups = [];
    var labelName = null;
    for (var b = 0; b < blocks.length; b++) {
      var block = blocks[b];
      if (!block.rows.length) continue;

      var columnNames = [];
      var autoFlags = [];
      var blockLabelName = null;
      if (block.header) {
        if (block.header.length === block.rows[0].values.length + 1) {
          blockLabelName = block.header[0];
          columnNames = block.header.slice(1);
        } else if (block.header.length === block.rows[0].values.length) {
          columnNames = block.header.slice();
        } else {
          throw new Error("表头列数与数据列数不一致");
        }
        for (var ha = 0; ha < columnNames.length; ha++) autoFlags.push(false);
      } else {
        var count = block.rows[0].values.length;
        columnNames = [];
        for (var c = 0; c < count; c++) {
          if (fallbackNames && fallbackNames[c]) {
            columnNames.push(fallbackNames[c]);
            autoFlags.push(!!fallbackIsAuto);
          } else {
            columnNames.push("列 " + (c + 1));
            autoFlags.push(true);
          }
        }
      }
      if (labelName === null) labelName = blockLabelName;

      groups.push({
        title: block.title === undefined ? null : block.title,
        labelName: blockLabelName,
        columns: inferColumns(block.rows, columnNames, autoFlags),
        rows: block.rows
      });
    }

    if (!groups.length) throw new Error("表格里没有数据行");
    return { groups: groups, notes: notes, labelName: labelName };
  }

  function parseColumns(text) {
    if (typeof text !== "string") {
      throw new Error("表格内容必须是字符串");
    }

    var sourceLines = text.split(/\r?\n/);
    var notes = [];
    var blocks = [];
    var header = null;
    var valueCount = null;
    var dataStarted = false;
    var current = null;

    for (var i = 0; i < sourceLines.length; i++) {
      var line = sourceLines[i].replace(/\s+$/, "");
      if (!line.trim()) continue;
      var cells = line.trim().split(/\s{2,}/);

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
          dataStarted = true;
          if (valueCount === null) valueCount = parsed.length;
          if (parsed.length !== valueCount) {
            throw new Error("每行的值列数不一致");
          }
          if (!current) {
            current = { title: null, header: header, rows: [] };
            blocks.push(current);
          }
          current.rows.push({ label: cells[0], values: parsed });
        } else if (!dataStarted) {
          header = cells;
        } else {
          throw new Error("数据行之后出现了无法解析的行：" + line);
        }
      } else {
        if (header || dataStarted) {
          current = {
            title: stripGroupTitle(cells[0]),
            header: header,
            rows: []
          };
          blocks.push(current);
        } else {
          notes.push(cells[0]);
        }
      }
    }

    if (!blocks.length) throw new Error("表格为空");
    return finishGroups(blocks, notes);
  }

  function parseLead(text, config) {
    if (typeof text !== "string") {
      throw new Error("表格内容必须是字符串");
    }
    config = config || {};

    var sourceLines = text.split(/\r?\n/);
    var blocks = [];
    var current = null;
    var valueCount = null;

    for (var i = 0; i < sourceLines.length; i++) {
      var line = sourceLines[i].replace(/\s+$/, "");
      if (!line.trim()) continue;
      var cells = line.trim().split(/\s{2,}/);
      var values = [];
      var index = 0;

      while (index < cells.length) {
        var match = VALUE_CELL.exec(cells[index]);
        if (!match) break;
        var unit = match[2] || "";
        if (unit && !LEAD_UNIT.test(unit)) break;
        values.push(makeCell(match[1], unit));
        index++;
      }

      if (!values.length) {
        current = {
          title: stripGroupTitle(line.trim()),
          header: null,
          rows: []
        };
        blocks.push(current);
        continue;
      }

      var label = cells.slice(index).join("  ").trim();
      if (!label) {
        throw new Error("数据行缺少标签：" + line);
      }
      if (valueCount === null) valueCount = values.length;
      if (values.length !== valueCount) {
        throw new Error("每行的值列数不一致：" + line);
      }
      if (!current) {
        current = { title: null, header: null, rows: [] };
        blocks.push(current);
      }
      current.rows.push({ label: label, values: values });
    }

    if (!blocks.length) throw new Error("表格为空");

    var names = [];
    var autoNames = false;
    if (config.columns !== undefined) {
      if (!Array.isArray(config.columns)) {
        throw new Error("columns 必须是数组");
      }
      if (config.columns.length !== valueCount) {
        throw new Error(
          "columns 有 " + config.columns.length + " 列，数据有 " + valueCount + " 列"
        );
      }
      names = config.columns.slice();
    } else {
      autoNames = true;
      for (var n = 0; n < valueCount; n++) names.push(defaultColumnName(n));
    }

    return finishGroups(blocks, [], names, autoNames);
  }

  function parseLaps(text) {
    if (typeof text !== "string") {
      throw new Error("分步耗时内容必须是字符串");
    }

    var pattern = /^\s*(\d+(?:\.\d+)?)\s*(ms|us|s)\s{2,}(.+?)\s*$/;
    var sourceLines = text.split(/\r?\n/);
    var laps = [];
    for (var i = 0; i < sourceLines.length; i++) {
      var match = pattern.exec(sourceLines[i]);
      if (!match) continue;
      laps.push({
        value: Number(match[1]),
        text: match[1],
        unit: match[2],
        label: match[3]
      });
    }

    if (!laps.length) throw new Error("没有可解析的分步耗时");
    return laps;
  }

  function trimFixed(value) {
    return Number(value).toFixed(3).replace(/\.?0+$/, "");
  }

  function assembleLapRows(laps, rowsConfig) {
    if (!Array.isArray(laps)) {
      throw new Error("分步耗时必须是数组");
    }
    if (!Array.isArray(rowsConfig) || !rowsConfig.length) {
      throw new Error("laps 格式缺少 rows 配置");
    }

    var rows = [];
    for (var i = 0; i < rowsConfig.length; i++) {
      var configRow = rowsConfig[i] || {};
      var labels = Array.isArray(configRow.laps) ? configRow.laps : [];
      if (!labels.length) {
        throw new Error("行 " + (configRow.label || i + 1) + " 没有 laps");
      }

      var matched = [];
      var total = 0;
      for (var j = 0; j < labels.length; j++) {
        var found = null;
        for (var k = 0; k < laps.length; k++) {
          if (laps[k].label === labels[j]) {
            found = laps[k];
            break;
          }
        }
        if (!found) throw new Error("找不到步骤：" + labels[j]);
        matched.push(found);
        total += found.value;
      }

      rows.push({
        label:
          configRow.label !== undefined ? configRow.label : matched[0].label,
        values: [
          {
            value: total,
            text: matched.length === 1 ? matched[0].text : trimFixed(total),
            unit: matched[0].unit
          }
        ],
        group: configRow.group,
        note: configRow.note
      });
    }
    return rows;
  }

  function findPickLine(lines, needle, after) {
    var start = 0;
    if (after !== undefined && after !== null) {
      var afterAt = -1;
      for (var a = 0; a < lines.length; a++) {
        if (lines[a].indexOf(after) !== -1) {
          afterAt = a;
          break;
        }
      }
      if (afterAt < 0) throw new Error("找不到 after 指定的行：" + after);
      start = afterAt + 1;
    }
    for (var i = start; i < lines.length; i++) {
      if (lines[i].indexOf(needle) !== -1) return lines[i];
    }
    throw new Error("找不到行：" + needle);
  }

  function pickLineTokens(line) {
    var matches = line.match(PICK_NUMBER) || [];
    var tokens = [];
    for (var t = 0; t < matches.length; t++) {
      tokens.push(parsePickedToken(matches[t]));
    }
    return tokens;
  }

  function pickIndex(container, label) {
    var raw = container.skip === undefined ? 0 : Number(container.skip);
    if (!isFinite(raw) || raw < 0) {
      throw new Error("pick 行 " + label + " 的 skip 无效");
    }
    return raw;
  }

  function validatePickConfig(config) {
    if (!config || !Array.isArray(config.groups) || !config.groups.length) {
      throw new Error("pick 格式缺少 groups 配置");
    }
    for (var g = 0; g < config.groups.length; g++) {
      var groupConfig = config.groups[g] || {};
      var rowConfigs = Array.isArray(groupConfig.rows) ? groupConfig.rows : [];
      for (var r = 0; r < rowConfigs.length; r++) {
        var rowConfig = rowConfigs[r] || {};
        var rowLabel =
          rowConfig.label !== undefined ? rowConfig.label : "第 " + (r + 1) + " 行";
        if (Array.isArray(rowConfig.cells)) {
          if (rowConfig.line !== undefined) {
            throw new Error("pick 行 " + rowLabel + " 同时写了 line 和 cells");
          }
          if (!rowConfig.cells.length) {
            throw new Error("pick 行 " + rowLabel + " 的 cells 是空的");
          }
          if (
            Array.isArray(config.columns) &&
            config.columns.length !== rowConfig.cells.length
          ) {
            throw new Error(
              "pick 行 " +
                rowLabel +
                " 有 " +
                rowConfig.cells.length +
                " 个 cell，columns 有 " +
                config.columns.length +
                " 列"
            );
          }
          for (var c = 0; c < rowConfig.cells.length; c++) {
            var cell = rowConfig.cells[c];
            if (isMissingCell(cell)) continue;
            if (typeof cell !== "object") {
              throw new Error(
                "pick 行 " + rowLabel + " 的第 " + (c + 1) + " 个 cell 格式无效"
              );
            }
            if (typeof cell.line !== "string") {
              throw new Error(
                "pick 行 " + rowLabel + " 的第 " + (c + 1) + " 个 cell 缺少 line"
              );
            }
            if (
              cell.n === undefined ||
              typeof cell.n !== "number" ||
              !isFinite(cell.n) ||
              cell.n < 0 ||
              Math.floor(cell.n) !== cell.n
            ) {
              throw new Error(
                "pick 行 " + rowLabel + " 的第 " + (c + 1) + " 个 cell 缺少有效的 n"
              );
            }
            pickIndex(cell, rowLabel + " 的第 " + (c + 1) + " 个 cell");
          }
        } else {
          if (typeof rowConfig.line !== "string") {
            throw new Error("pick 的第 " + (r + 1) + " 行缺少 line");
          }
          if (!Array.isArray(rowConfig.n)) {
            throw new Error("pick 行 " + rowLabel + " 缺少 n");
          }
        }
      }
    }
  }

  function pickCell(lines, cell, rowLabel, cellIndex) {
    var where = "pick 行 " + rowLabel + " 的第 " + cellIndex + " 个 cell";
    var line;
    try {
      line = findPickLine(lines, cell.line, cell.after);
    } catch (error) {
      throw new Error(where + " " + error.message);
    }
    var tokens = pickLineTokens(line);
    var at = pickIndex(cell, where) + cell.n;
    if (
      !isFinite(at) ||
      at < 0 ||
      at >= tokens.length ||
      Math.floor(at) !== at
    ) {
      throw new Error(where + " 取不到第 " + cell.n + " 个数值（行：" + line.trim() + "）");
    }
    return tokens[at];
  }

  function pickRows(config, text) {
    if (typeof text !== "string") {
      throw new Error("表格内容必须是字符串");
    }
    validatePickConfig(config);

    var lines = text.split(/\r?\n/);
    var valueCount = null;
    var groups = [];

    for (var g = 0; g < config.groups.length; g++) {
      var groupConfig = config.groups[g] || {};
      var rows = [];
      var rowConfigs = Array.isArray(groupConfig.rows)
        ? groupConfig.rows
        : [];

      for (var r = 0; r < rowConfigs.length; r++) {
        var rowConfig = rowConfigs[r] || {};
        var rowLabel =
          rowConfig.label !== undefined ? rowConfig.label : "第 " + (r + 1) + " 行";
        var values = [];

        if (Array.isArray(rowConfig.cells)) {
          for (var c = 0; c < rowConfig.cells.length; c++) {
            var cell = rowConfig.cells[c];
            if (isMissingCell(cell)) {
              values.push(null);
            } else {
              values.push(pickCell(lines, cell, rowLabel, c + 1));
            }
          }
        } else {
          var line = findPickLine(lines, rowConfig.line, rowConfig.after);
          var tokens = pickLineTokens(line);
          var skip = pickIndex(rowConfig, rowLabel);
          for (var n = 0; n < rowConfig.n.length; n++) {
            var at = skip + Number(rowConfig.n[n]);
            if (!isFinite(at) || at < 0 || at >= tokens.length) {
              throw new Error(
                "pick 行 " +
                  rowLabel +
                  " 取不到第 " +
                  rowConfig.n[n] +
                  " 个数值"
              );
            }
            values.push(tokens[at]);
          }
        }

        if (valueCount === null) valueCount = values.length;
        if (values.length !== valueCount) {
          throw new Error("pick 每行的值列数不一致");
        }
        rows.push({
          label: rowLabel,
          values: values,
          note: rowConfig.note
        });
      }

      groups.push({
        title: groupConfig.title === undefined ? null : groupConfig.title,
        rows: rows
      });
    }

    var names = [];
    var autoNames = false;
    if (config.columns !== undefined) {
      if (!Array.isArray(config.columns)) {
        throw new Error("columns 必须是数组");
      }
      if (config.columns.length !== valueCount) {
        throw new Error(
          "columns 有 " + config.columns.length + " 列，数据有 " + valueCount + " 列"
        );
      }
      names = config.columns.slice();
    } else {
      autoNames = true;
      for (var i = 0; i < valueCount; i++) names.push(defaultColumnName(i));
    }

    var blocks = groups.map(function (group) {
      return { title: group.title, header: null, rows: group.rows };
    });
    return finishGroups(blocks, [], names, autoNames);
  }

  function extractTable(config, sourceText) {
    if (!config || typeof config !== "object") {
      throw new Error("缺少图表配置");
    }
    var format = config.format || "columns";

    if (format === "columns") return parseColumns(sourceText);
    if (format === "lead") return parseLead(sourceText, config);
    if (format === "pick") return pickRows(config, sourceText);
    if (format === "laps") {
      var laps = parseLaps(sourceText);
      var rows = assembleLapRows(laps, config.rows);
      var unit = config.unit || (laps[0] && laps[0].unit);
      var name = config.panel || "耗时";
      var columns = [{ name: name, unit: unit }];
      return {
        groups: [{ title: null, labelName: null, columns: columns, rows: rows }],
        notes: [],
        labelName: null
      };
    }

    throw new Error("未知的 format：" + format);
  }

  function parseXValue(raw) {
    if (typeof raw === "number") {
      if (!isFinite(raw)) throw new Error("x 不是数字：" + raw);
      return raw;
    }
    var text = String(raw).trim();
    var match = /^(-?\d[\d,]*(?:\.\d+)?(?:e[-+]?\d+)?)\s*([A-Za-zµ%]+)?/.exec(
      text
    );
    if (!match) throw new Error("x 不是数字：" + raw);

    var value = Number(match[1].replace(/,/g, ""));
    var unit = (match[2] || "").toUpperCase();
    var factors = {
      B: 1,
      KB: 1024,
      MB: 1024 * 1024,
      GB: 1024 * 1024 * 1024,
      TB: 1024 * 1024 * 1024 * 1024
    };
    if (Object.prototype.hasOwnProperty.call(factors, unit)) {
      value *= factors[unit];
    }
    return value;
  }

  function resolveValueIndices(config, group) {
    if (config.value === undefined || config.value === null) return null;
    var spec = config.value;
    if (typeof spec === "number") {
      if (!Number.isInteger(spec) || spec < 0 || spec >= group.columns.length) {
        throw new Error("value 列下标超出范围：" + spec);
      }
      return [spec];
    }
    if (typeof spec === "string") {
      for (var i = 0; i < group.columns.length; i++) {
        if (group.columns[i].name === spec) return [i];
      }
      throw new Error("找不到 value 列：" + spec);
    }
    throw new Error("value 必须是列名或列下标");
  }

  function resolveColumnIndex(group, name) {
    for (var i = 0; i < group.columns.length; i++) {
      if (group.columns[i].name === name) return i;
    }
    throw new Error("找不到列：" + name);
  }

  function validateExtraSeries(config, table) {
    if (config.extraSeries === undefined) return;
    if (!Array.isArray(config.extraSeries)) {
      throw new Error("extraSeries 必须是数组");
    }
    var totalRows = 0;
    for (var g = 0; g < table.groups.length; g++) {
      totalRows += table.groups[g].rows.length;
    }
    var baseCount =
      Array.isArray(config.series) && config.series.length
        ? config.series.length
        : 1;
    if (baseCount + config.extraSeries.length > SERIES_CLASSES.length) {
      throw new Error("折线图最多 4 条线（含 extraSeries）");
    }
    for (var e = 0; e < config.extraSeries.length; e++) {
      var extra = config.extraSeries[e] || {};
      if (typeof extra.name !== "string" || !extra.name) {
        throw new Error("extraSeries 第 " + (e + 1) + " 项缺少 name");
      }
      if (!Array.isArray(extra.values)) {
        throw new Error("extraSeries 的 " + extra.name + " 缺少 values");
      }
      if (extra.values.length !== totalRows) {
        throw new Error(
          "extraSeries 的 " +
            extra.name +
            " 有 " +
            extra.values.length +
            " 个值，数据有 " +
            totalRows +
            " 行"
        );
      }
      for (var v = 0; v < extra.values.length; v++) {
        var value = Number(extra.values[v]);
        if (!isFinite(value)) {
          throw new Error(
            "extraSeries 的 " + extra.name + " 第 " + (v + 1) + " 个值不是数字"
          );
        }
      }
      if (extra.dashed !== undefined && typeof extra.dashed !== "boolean") {
        throw new Error("extraSeries 的 " + extra.name + " 的 dashed 必须是布尔值");
      }
    }
  }

  function validateRooflineConfig(config) {
    var peak = Number(config.peak);
    if (!isFinite(peak) || peak <= 0) {
      throw new Error("roofline 的 peak 必须是正数");
    }
    if (config.peakUnit !== undefined && config.peakUnit !== "TFLOPS" && config.peakUnit !== "GFLOPS") {
      throw new Error("roofline 的 peakUnit 只能是 TFLOPS 或 GFLOPS");
    }
    var bandwidth = Number(config.bandwidth);
    if (!isFinite(bandwidth) || bandwidth <= 0) {
      throw new Error("roofline 的 bandwidth 必须是正数");
    }
    if (!Array.isArray(config.points) || !config.points.length) {
      throw new Error("roofline 没有 points");
    }
    for (var p = 0; p < config.points.length; p++) {
      var point = config.points[p] || {};
      if (typeof point.label !== "string" || !point.label) {
        throw new Error("roofline 第 " + (p + 1) + " 个点缺少 label");
      }
      var x = Number(point.x);
      var y = Number(point.y);
      if (!isFinite(x) || x <= 0 || !isFinite(y) || y <= 0) {
        throw new Error("roofline 的 " + point.label + " 坐标必须是正数");
      }
      if (point.note !== undefined && typeof point.note !== "string") {
        throw new Error("roofline 的 " + point.label + " 的 note 必须是字符串");
      }
    }
    var ranges = [
      ["xMin", "xMax"],
      ["yMin", "yMax"]
    ];
    for (var r = 0; r < ranges.length; r++) {
      var lo = config[ranges[r][0]];
      var hi = config[ranges[r][1]];
      if (lo !== undefined) {
        if (!isFinite(Number(lo)) || Number(lo) <= 0) {
          throw new Error("roofline 的 " + ranges[r][0] + " 必须是正数");
        }
      }
      if (hi !== undefined) {
        if (!isFinite(Number(hi)) || Number(hi) <= 0) {
          throw new Error("roofline 的 " + ranges[r][1] + " 必须是正数");
        }
      }
      if (lo !== undefined && hi !== undefined && Number(hi) <= Number(lo)) {
        throw new Error(
          "roofline 的 " + ranges[r][1] + " 必须大于 " + ranges[r][0]
        );
      }
    }
  }

  function chartDisplayName(config, context) {
    if (context) return String(context);
    if (config && typeof config.title === "string" && config.title) {
      return "图表（" + config.title + "）";
    }
    if (config && typeof config.yLabel === "string" && config.yLabel) {
      return "图表（" + config.yLabel + "）";
    }
    return "图表";
  }

  function missingCellError(config, context, row, rowIndex, colIndex, colName) {
    return (
      chartDisplayName(config, context) +
      " 第 " +
      (rowIndex + 1) +
      " 行（" +
      row.label +
      "）第 " +
      (colIndex + 1) +
      " 列（" +
      colName +
      "）是缺值；只有折线图支持缺值（kind: \"line\"）"
    );
  }

  function findMissingCell(table) {
    for (var g = 0; g < table.groups.length; g++) {
      var group = table.groups[g];
      for (var r = 0; r < group.rows.length; r++) {
        for (var c = 0; c < group.rows[r].values.length; c++) {
          if (isMissingCell(group.rows[r].values[c])) {
            return {
              row: group.rows[r],
              rowIndex: r,
              columnIndex: c,
              columnName:
                group.columns[c] && group.columns[c].name
                  ? group.columns[c].name
                  : "列 " + (c + 1)
            };
          }
        }
      }
    }
    return null;
  }

  function rejectMissingForNonLine(config, table, context) {
    if (!table) return;
    var missing = findMissingCell(table);
    if (!missing) return;
    throw new Error(
      missingCellError(
        config,
        context,
        missing.row,
        missing.rowIndex,
        missing.columnIndex,
        missing.columnName
      )
    );
  }

  function validateChart(config, table, context) {
    if (!config || typeof config !== "object") {
      throw new Error("缺少图表配置");
    }
    var kind = config.kind;
    if (kind === "roofline") {
      validateRooflineConfig(config);
      return true;
    }
    if (kind !== "bars" && kind !== "line" && kind !== "balance") {
      throw new Error("kind 必须是 bars、line、balance 或 roofline");
    }

    if (kind === "balance") {
      rejectMissingForNonLine(config, table, context);
      validateBalanceConfig(config, context);
      return true;
    }

    if (!table || !Array.isArray(table.groups) || !table.groups.length) {
      throw new Error("表格没有分组");
    }
    if (kind !== "line") rejectMissingForNonLine(config, table, context);

    var format = config.format || "columns";
    if (
      format !== "columns" &&
      format !== "lead" &&
      format !== "laps" &&
      format !== "pick"
    ) {
      throw new Error("未知的 format：" + format);
    }
    if (format === "pick") validatePickConfig(config);

    if (!config || typeof config !== "object") {
      throw new Error("缺少图表配置");
    }
    var kind = config.kind;
    if (kind !== "bars" && kind !== "line" && kind !== "balance") {
      throw new Error("kind 必须是 bars、line 或 balance");
    }

    if (kind === "balance") {
      validateBalanceConfig(config, context);
      return true;
    }

    if (!table || !Array.isArray(table.groups) || !table.groups.length) {
      throw new Error("表格没有分组");
    }

    var format = config.format || "columns";
    if (
      format !== "columns" &&
      format !== "lead" &&
      format !== "laps" &&
      format !== "pick"
    ) {
      throw new Error("未知的 format：" + format);
    }

    var series = null;
    if (config.series !== undefined) {
      if (!Array.isArray(config.series) || !config.series.length) {
        throw new Error("series 必须是非空数组");
      }
      if (config.series.length > SERIES_CLASSES.length) {
        throw new Error("series 最多 4 个");
      }
      for (var s = 0; s < config.series.length; s++) {
        if (typeof config.series[s] !== "string") {
          throw new Error("series 里必须是列名");
        }
      }
      series = config.series;
    }

    if (
      config.color !== undefined &&
      config.color !== "first-word" &&
      config.color !== "single" &&
      config.color !== "rows" &&
      config.color !== "series"
    ) {
      throw new Error("未知的 color：" + config.color);
    }
    if (config.color === "series" && !series) {
      throw new Error('color: "series" 需要同时写 series 列表');
    }
    if (config.color === "rows") {
      if (!config.rowGroups || typeof config.rowGroups !== "object") {
        throw new Error('color: "rows" 需要 rowGroups');
      }
    }
    if (config.rowGroups !== undefined) {
      if (!config.rowGroups || typeof config.rowGroups !== "object") {
        throw new Error("rowGroups 必须是对象");
      }
    }
    if (config.notes !== undefined) {
      if (!config.notes || typeof config.notes !== "object") {
        throw new Error("notes 必须是对象");
      }
    }
    if (config.baseline !== undefined) {
      var foundBaseline = false;
      for (var gb = 0; gb < table.groups.length; gb++) {
        for (var rb = 0; rb < table.groups[gb].rows.length; rb++) {
          if (table.groups[gb].rows[rb].label === config.baseline) {
            foundBaseline = true;
          }
        }
      }
      if (!foundBaseline) throw new Error("找不到 baseline 行：" + config.baseline);
    }

    if (kind === "bars") {
      if (config.log !== undefined && typeof config.log !== "boolean") {
        throw new Error("log 必须是布尔值");
      }
      if (
        config.panels !== undefined &&
        config.panels !== "groups" &&
        (typeof config.panels !== "object" || config.panels === null)
      ) {
        throw new Error("panels 只能是 \"groups\" 或 列名到说明的对象");
      }
      if (config.refs !== undefined) {
        if (!Array.isArray(config.refs)) throw new Error("bars 的 refs 必须是数组");
        for (var br = 0; br < config.refs.length; br++) {
          var barRef = config.refs[br] || {};
          var refX = Number(barRef.x);
          if (!isFinite(refX)) {
            throw new Error("bars 的 refs 第 " + (br + 1) + " 项缺少数字 x");
          }
          if (config.log && refX <= 0) {
            throw new Error("对数柱状图的参考线 x 必须大于 0");
          }
          if (barRef.label !== undefined && typeof barRef.label !== "string") {
            throw new Error("bars 的 refs 第 " + (br + 1) + " 项的 label 必须是字符串");
          }
        }
      }
      for (var g = 0; g < table.groups.length; g++) {
        var group = table.groups[g];
        var logIndices = [];
        var groupsMode =
          config.panels === "groups" ||
          (table.groups.length > 1 && config.value === undefined);
        if (series) {
          for (var sn = 0; sn < series.length; sn++) {
            logIndices.push(resolveColumnIndex(group, series[sn]));
          }
        } else if (config.value !== undefined) {
          logIndices = resolveValueIndices(config, group);
        } else if (groupsMode) {
          logIndices.push(0);
        } else {
          for (var lc0 = 0; lc0 < group.columns.length; lc0++) {
            logIndices.push(lc0);
          }
        }
        if (config.log) {
          for (var lr = 0; lr < group.rows.length; lr++) {
            for (var lc = 0; lc < logIndices.length; lc++) {
              if (group.rows[lr].values[logIndices[lc]].value <= 0) {
                throw new Error("log 柱状图要求数值大于 0：" + group.rows[lr].label);
              }
            }
          }
        }
        if (config.color === "rows") {
          for (var rr = 0; rr < group.rows.length; rr++) {
            if (!Object.prototype.hasOwnProperty.call(config.rowGroups, group.rows[rr].label)) {
              throw new Error("rowGroups 里缺少行标签：" + group.rows[rr].label);
            }
          }
        }
      }
    } else {
      if (
        config.xScale !== undefined &&
        config.xScale !== "log2" &&
        config.xScale !== "log10" &&
        config.xScale !== "linear"
      ) {
        throw new Error("xScale 只能是 log2、log10 或 linear");
      }
      if (
        config.yScale !== undefined &&
        config.yScale !== "linear" &&
        config.yScale !== "log"
      ) {
        throw new Error("yScale 只能是 linear 或 log");
      }
      if (
        config.xFormat !== undefined &&
        config.xFormat !== "count" &&
        config.xFormat !== "bytes" &&
        config.xFormat !== "plain"
      ) {
        throw new Error("xFormat 只能是 count、bytes 或 plain");
      }
      if (config.xName !== undefined && typeof config.xName !== "string") {
        throw new Error("xName 必须是字符串");
      }
      if (config.logToggle !== undefined && typeof config.logToggle !== "boolean") {
        throw new Error("logToggle 必须是布尔值");
      }
      if (config.refs !== undefined) {
        if (!Array.isArray(config.refs)) throw new Error("line 的 refs 必须是数组");
        for (var lr0 = 0; lr0 < config.refs.length; lr0++) {
          var lineRef = config.refs[lr0] || {};
          var refY = Number(lineRef.y);
          if (!isFinite(refY)) {
            throw new Error("line 的 refs 第 " + (lr0 + 1) + " 项缺少数字 y");
          }
          if (config.yScale === "log" && refY <= 0) {
            throw new Error("对数纵轴的参考线 y 必须大于 0");
          }
          if (lineRef.label !== undefined && typeof lineRef.label !== "string") {
            throw new Error("line 的 refs 第 " + (lr0 + 1) + " 项的 label 必须是字符串");
          }
        }
      }
      for (var gl = 0; gl < table.groups.length; gl++) {
        var lineGroup = table.groups[gl];
        if (config.x !== undefined) {
          if (typeof config.x !== "string") throw new Error("x 必须是列名");
          if (config.x !== lineGroup.labelName) {
            resolveColumnIndex(lineGroup, config.x);
          }
        }
        var lineIndices = [];
        if (series) {
          for (var ls = 0; ls < series.length; ls++) {
            lineIndices.push(resolveColumnIndex(lineGroup, series[ls]));
          }
        } else {
          lineIndices.push(0);
        }
        for (var lv = 0; lv < lineIndices.length; lv++) {
          var hasValue = false;
          for (var lvr = 0; lvr < lineGroup.rows.length; lvr++) {
            if (
              !isMissingCell(
                lineGroup.rows[lvr].values[lineIndices[lv]]
              )
            ) {
              hasValue = true;
              break;
            }
          }
          if (!hasValue) {
            var emptyColumn = lineGroup.columns[lineIndices[lv]];
            throw new Error(
              chartDisplayName(config, context) +
                " 的系列 " +
                (emptyColumn && emptyColumn.name
                  ? emptyColumn.name
                  : "列 " + (lineIndices[lv] + 1)) +
                " 全是缺值；折线图每条线至少要有一个有值的点"
            );
          }
        }
        for (var lrow = 0; lrow < lineGroup.rows.length; lrow++) {
          var xRaw;
          if (config.x !== undefined) {
            if (config.x === lineGroup.labelName) {
              xRaw = lineGroup.rows[lrow].label;
            } else {
              var xIndex = resolveColumnIndex(lineGroup, config.x);
              var xCell = lineGroup.rows[lrow].values[xIndex];
              if (isMissingCell(xCell)) {
                throw new Error(
                  chartDisplayName(config, context) +
                    " 第 " +
                    (lrow + 1) +
                    " 行（" +
                    lineGroup.rows[lrow].label +
                    "）的 x 列（" +
                    config.x +
                    "）是缺值"
                );
              }
              xRaw = xCell.text + (xCell.unit ? " " + xCell.unit : "");
            }
          } else {
            xRaw = lineGroup.rows[lrow].label;
          }
          var xValue = parseXValue(xRaw);
          if (
            config.xScale !== undefined &&
            config.xScale !== "linear" &&
            xValue <= 0
          ) {
            throw new Error("对数横轴要求 x 大于 0：" + xRaw);
          }
          if (config.yScale === "log") {
            for (var lc2 = 0; lc2 < lineIndices.length; lc2++) {
              var logCell = lineGroup.rows[lrow].values[lineIndices[lc2]];
              if (!isMissingCell(logCell) && logCell.value <= 0) {
                throw new Error(
                  "对数纵轴要求数值大于 0：" + lineGroup.rows[lrow].label
                );
              }
            }
          }
        }
      }
      validateExtraSeries(config, table);
      if (config.share && config.share.of !== undefined) {
        var shareOf = Number(config.share.of);
        if (!isFinite(shareOf) || shareOf <= 0) {
          throw new Error("share.of 必须是正数");
        }
      }
    }

    return true;
  }

  function validateBalanceConfig(config, context) {
    var rows = config.rows;
    if (!Array.isArray(rows) || !rows.length) {
      throw new Error("balance 图没有 rows");
    }
    for (var br = 0; br < rows.length; br++) {
      var balanceRow = rows[br] || {};
      if (isMissingCell(balanceRow.balance)) {
        throw new Error(
          chartDisplayName(config, context) +
            " 第 " +
            (br + 1) +
            " 行（" +
            (balanceRow.label === undefined ? "" : balanceRow.label) +
            "）的 balance 列是缺值；只有折线图支持缺值（kind: \"line\"）"
        );
      }
    }
    var domain = config.domain;
    if (!Array.isArray(domain) || domain.length !== 2) {
      throw new Error("balance 图缺少 domain");
    }
    var lo = Number(domain[0]);
    var hi = Number(domain[1]);
    if (!isFinite(lo) || !isFinite(hi) || lo <= 0 || hi <= lo) {
      throw new Error("balance 图的 domain 必须是从小到大的正数");
    }
    if (config.tasks !== undefined && !Array.isArray(config.tasks)) {
      throw new Error("balance 图的 tasks 必须是数组");
    }
  }

  function log10Ticks(lo, hi) {
    lo = Number(lo);
    hi = Number(hi);
    if (!isFinite(lo) || !isFinite(hi) || lo <= 0 || hi <= 0 || hi < lo) {
      return [];
    }

    var first = Math.ceil(Math.log10(lo) - 1e-12);
    var last = Math.floor(Math.log10(hi) + 1e-12);
    var ticks = [];
    for (var power = first; power <= last; power++) {
      ticks.push(Number(Math.pow(10, power).toPrecision(12)));
    }
    return ticks;
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

  function formatBytes(n) {
    var value = Number(n);
    if (!isFinite(value)) return String(n);
    if (Math.abs(value) >= 1024 * 1024 * 1024) {
      return trimUnitValue(value / (1024 * 1024 * 1024)) + " GB";
    }
    if (Math.abs(value) >= 1024 * 1024) {
      return trimUnitValue(value / (1024 * 1024)) + " MB";
    }
    if (Math.abs(value) >= 1024) {
      return trimUnitValue(value / 1024) + " KB";
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

  // 大数刻度的统一缩写：按这根轴上最大的刻度选一个后缀，整根轴共用。
  function magnitudeScale(maxAbs) {
    var abs = Math.abs(Number(maxAbs));
    if (!isFinite(abs)) return { divisor: 1, suffix: "" };
    if (abs >= 1e12) return { divisor: 1e12, suffix: "T" };
    if (abs >= 1e9) return { divisor: 1e9, suffix: "G" };
    if (abs >= 1e6) return { divisor: 1e6, suffix: "M" };
    if (abs >= 1e4) return { divisor: 1e3, suffix: "K" };
    return { divisor: 1, suffix: "" };
  }

  function formatMagnitudeTick(value, scale) {
    if (!scale || !scale.suffix || scale.divisor === 1) return formatTick(value);
    if (Number(value) === 0) return "0";
    return formatTick(Number(value) / scale.divisor) + scale.suffix;
  }

  function niceDecimalStep(rawStep) {
    var raw = Number(rawStep);
    if (!isFinite(raw) || raw <= 0) return 1;
    var exponent = Math.floor(Math.log10(raw));
    var magnitude = Math.pow(10, exponent);
    var fraction = raw / magnitude;
    var nice;
    if (fraction <= 1) nice = 1;
    else if (fraction <= 2) nice = 2;
    else if (fraction <= 5) nice = 5;
    else nice = 10;
    return nice * magnitude;
  }

  // 字节横轴用线性刻度时，按 GB/MB/KB 取 1、2、5 × 10^k 的整齐步长。
  function byteLinearTicks(min, max, approxCount) {
    var lo = Number(min);
    var hi = Number(max);
    var count = Number(approxCount);
    if (!isFinite(count) || count <= 0) count = 5;
    if (!isFinite(lo) || !isFinite(hi) || hi <= lo) return [lo];
    var span = hi - lo;
    var unit = 1;
    if (span >= 1024 * 1024 * 1024) unit = 1024 * 1024 * 1024;
    else if (span >= 1024 * 1024) unit = 1024 * 1024;
    else if (span >= 1024) unit = 1024;
    var step = niceDecimalStep(span / unit / count) * unit;
    if (!isFinite(step) || step <= 0) return [lo, hi];
    var first = Math.ceil(lo / step - 1e-9) * step;
    var ticks = [];
    for (var value = first; value <= hi + 1e-9 && ticks.length < 60; value += step) {
      ticks.push(Number(value.toPrecision(12)));
    }
    if (!ticks.length) ticks = [lo, hi];
    return ticks;
  }

  // peak 用 TFLOPS，bandwidth 用 GB/s；返回同样以 TFLOPS 计的上限。
  function rooflineLimit(x, peak, bandwidth) {
    var intensity = Number(x);
    var peakValue = Number(peak);
    var bandwidthValue = Number(bandwidth);
    if (
      !isFinite(intensity) ||
      !isFinite(peakValue) ||
      !isFinite(bandwidthValue)
    ) {
      return 0;
    }
    return Math.min(peakValue, (bandwidthValue * intensity) / 1000);
  }

  function rooflineBalance(peak, bandwidth) {
    var peakValue = Number(peak);
    var bandwidthValue = Number(bandwidth);
    if (!isFinite(peakValue) || !isFinite(bandwidthValue) || bandwidthValue <= 0) {
      return 0;
    }
    return (peakValue * 1000) / bandwidthValue;
  }

  // 线段和文字包围盒是否相交（Liang–Barsky）。
  function segmentIntersectsBox(x1, y1, x2, y2, box) {
    var dx = x2 - x1;
    var dy = y2 - y1;
    var t0 = 0;
    var t1 = 1;
    var p = [-dx, dx, -dy, dy];
    var q = [x1 - box.left, box.right - x1, y1 - box.top, box.bottom - y1];
    for (var i = 0; i < 4; i++) {
      if (Math.abs(p[i]) < 1e-12) {
        if (q[i] < 0) return false;
      } else {
        var ratio = q[i] / p[i];
        if (p[i] < 0) {
          if (ratio > t1) return false;
          if (ratio > t0) t0 = ratio;
        } else {
          if (ratio < t0) return false;
          if (ratio < t1) t1 = ratio;
        }
      }
    }
    return true;
  }

  function boxBlocked(box, obstacles) {
    for (var i = 0; i < obstacles.length; i++) {
      var item = obstacles[i];
      if (!item) continue;
      if (item.box && boxesIntersect(box, item.box)) return true;
      if (
        item.segment &&
        segmentIntersectsBox(
          item.segment[0],
          item.segment[1],
          item.segment[2],
          item.segment[3],
          box
        )
      ) {
        return true;
      }
    }
    return false;
  }

  // 末端标签最多上下挪一个字高：0、上移、下移，仍被挡住就返回 null（不画）。
  function findLabelOffset(box, obstacles, step, bounds) {
    var shifts = [0, -step, step];
    for (var i = 0; i < shifts.length; i++) {
      var shifted = {
        left: box.left,
        right: box.right,
        top: box.top + shifts[i],
        bottom: box.bottom + shifts[i]
      };
      if (bounds) {
        if (shifted.top < bounds.top || shifted.bottom > bounds.bottom) continue;
      }
      if (!boxBlocked(shifted, obstacles)) return shifts[i];
    }
    return null;
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
      parseLead: parseLead,
      parseLaps: parseLaps,
      pickRows: pickRows,
      validatePickConfig: validatePickConfig,
      extractTable: extractTable,
      validateChart: validateChart,
      validateExtraSeries: validateExtraSeries,
      validateRooflineConfig: validateRooflineConfig,
      parseXValue: parseXValue,
      isMissingCell: isMissingCell,
      splitLineSegments: splitLineSegments,
      pointValueText: pointValueText,
      assembleLapRows: assembleLapRows,
      log10Ticks: log10Ticks,
      niceTicks: niceTicks,
      formatThreads: formatThreads,
      formatBytes: formatBytes,
      formatSpeedup: formatSpeedup,
      formatXValue: formatXValue,
      magnitudeScale: magnitudeScale,
      formatMagnitudeTick: formatMagnitudeTick,
      byteLinearTicks: byteLinearTicks,
      rooflineLimit: rooflineLimit,
      rooflineBalance: rooflineBalance,
      segmentIntersectsBox: segmentIntersectsBox,
      findLabelOffset: findLabelOffset,
      nearestIndex: nearestIndex,
      pointLabelOffset: pointLabelOffset,
      chooseReferenceLabelSide: chooseReferenceLabelSide
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

      var config = JSON.parse(script.textContent || "");
      if (
        !config ||
        (config.kind !== "bars" &&
          config.kind !== "line" &&
          config.kind !== "balance" &&
          config.kind !== "roofline")
      ) {
        throw new Error("kind 必须是 bars、line、balance 或 roofline");
      }
      var data = null;

      if (config.kind !== "balance" && config.kind !== "roofline") {
        if (!source) throw new Error("找不到 data-source 指向的元素");
        var code =
          source.tagName === "SCRIPT" ? source : source.querySelector("code");
        if (!code) throw new Error("data-source 既不是 script，下面也没有 code 元素");
        data = extractTable(config, code.textContent || "");
      }

      validateChart(config, data, captionText(fig));

      if (!fig.querySelector(".cu-chart-head")) {
        var tip = element("div", "cu-chart-tip");
        tip.setAttribute("aria-hidden", "true");

        var live = element("div", "cu-sr-only");
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
          failed: false,
          hide: function () {}
        };

        try {
          if (config.kind === "bars") initBars(state);
          else if (config.kind === "line") initLine(state);
          else if (config.kind === "roofline") initRoofline(state);
          else initBalance(state);

          state.render();
        } catch (renderError) {
          state.failed = true;
          throw renderError;
        }

        chartStates.push(state);
        watchSize(state);
      }
    } catch (error) {
      failFigure(fig, plot, error);
    }
  }

  function failFigure(fig, plot, error) {
    console.warn("[cu-chart] 图表渲染失败：", error);
    fig.classList.add("cu-chart-failed");
    var message = error && error.message ? error.message : String(error);
    var oldLabel = fig.querySelector(".cu-chart-xlabel");
    if (oldLabel && oldLabel.parentNode) oldLabel.parentNode.removeChild(oldLabel);
    if (!plot) return;
    while (plot.firstChild) plot.removeChild(plot.firstChild);
    plot.appendChild(
      element("div", "cu-chart-error", "图表数据解析失败：" + message)
    );
  }

  function watchSize(state) {
    if (typeof ResizeObserver !== "function") return;

    var frame = 0;
    var observer = new ResizeObserver(function () {
      if (frame || state.failed) return;
      frame = requestAnimationFrame(function () {
        frame = 0;
        var width = Math.round(state.plot.clientWidth);
        if (width < 1) return;
        if (Math.abs(width - state.width) >= 1) {
          try {
            state.render(width);
          } catch (error) {
            state.failed = true;
            failFigure(state.fig, state.plot, error);
            observer.disconnect();
          }
        }
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
    canvasContext.font =
      (weight || 400) + " " + size + "px " + (family || "sans-serif");
    return canvasContext.measureText(String(text)).width;
  }

  // 按字号估算文字包围盒；用于判断直接标注会不会压到参考线文字。
  function textBox(family, size, weight, text, x, y, anchor) {
    var width = textWidth(family, size, weight, text);
    var left = x;
    if (anchor === "end") left = x - width;
    else if (anchor === "middle") left = x - width / 2;
    return {
      left: left,
      right: left + width,
      top: y - size * 0.85,
      bottom: y + size * 0.3
    };
  }

  function boxesIntersect(a, b) {
    return (
      a.left < b.right - 0.5 &&
      b.left < a.right - 0.5 &&
      a.top < b.bottom - 0.5 &&
      b.top < a.bottom - 0.5
    );
  }

  function truncateText(family, size, weight, text, maxWidth) {
    text = String(text);
    if (!text || textWidth(family, size, weight, text) <= maxWidth) return text;

    var ellipsis = "…";
    var low = 0;
    var high = text.length;
    while (low < high) {
      var middle = Math.ceil((low + high) / 2);
      if (
        textWidth(family, size, weight, text.slice(0, middle) + ellipsis) <=
        maxWidth
      ) {
        low = middle;
      } else {
        high = middle - 1;
      }
    }
    return text.slice(0, low) + ellipsis;
  }

  // 按字符宽度折行，最多两行；第二行放剩余文字，不截断。
  function wrapText(family, size, weight, text, maxWidth, maxLines) {
    text = String(text);
    maxLines = maxLines || 2;
    if (!text || textWidth(family, size, weight, text) <= maxWidth) {
      return [text];
    }
    if (maxLines === 1) return [text];

    var lines = [];
    var rest = text;
    while (rest && lines.length < maxLines - 1) {
      var fit = 0;
      for (var i = 1; i <= rest.length; i++) {
        if (textWidth(family, size, weight, rest.slice(0, i)) <= maxWidth) {
          fit = i;
        } else {
          break;
        }
      }
      if (!fit) fit = 1;
      lines.push(rest.slice(0, fit));
      rest = rest.slice(fit);
    }
    if (rest) lines.push(rest);
    return lines;
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
      if (item.name !== undefined) {
        row.appendChild(element("span", "cu-tip-name", item.name));
      }
      if (item.value !== undefined) {
        row.appendChild(
          element("span", "cu-tip-text cu-tip-value", item.value)
        );
        spoken.push(
          (item.name !== undefined ? item.name + " " : "") + item.value
        );
      } else {
        row.appendChild(element("span", "cu-tip-text", item.text));
        spoken.push(
          (item.name !== undefined ? item.name + " " : "") + item.text
        );
      }
      tip.appendChild(row);
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

  function addXLabel(fig, plot, text) {
    var existing = fig.querySelector(".cu-chart-xlabel");
    if (!text) {
      if (existing && existing.parentNode) existing.parentNode.removeChild(existing);
      return;
    }
    if (!existing) existing = element("div", "cu-chart-xlabel");
    existing.textContent = text;
    if (existing.parentNode !== fig) {
      fig.insertBefore(existing, plot.nextSibling);
    }
  }

  function formatTick(value) {
    if (Number.isInteger(value)) return String(value);
    if (value !== 0 && Math.abs(value) < 1e-3) {
      return value.toExponential().replace(/\.?0+e/, "e").replace("e+", "e");
    }
    return String(Number(value.toFixed(4)));
  }

  function formatThousands(value) {
    return String(Math.round(value)).replace(/\B(?=(\d{3})+(?!\d))/g, ",");
  }

  function formatCount(value) {
    return formatThreads(value);
  }

  function formatAxisValue(value, format) {
    if (format === "bytes") return formatBytes(value);
    if (format === "plain") return formatTick(value);
    return formatCount(value);
  }

  // tooltip / aria 里的 x 值：bytes 和刻度一样带单位，count 保留千分位，plain 原样。
  function formatXValue(value, format) {
    if (format === "bytes") return formatBytes(value);
    if (format === "plain") return formatTick(value);
    return formatThousands(value);
  }

  // 首末点直接标注的位置。用屏幕坐标判断升降，对数纵轴也一样。
  // 首点：右上方/右下方；末点：左上方/左下方。
  function pointLabelOffset(role, currentY, neighborY) {
    var neighborHigher = neighborY < currentY;
    return {
      dx: role === "first" ? 6 : -6,
      dy: neighborHigher ? 16 : -9,
      anchor: role === "first" ? "start" : "end",
      vertical: neighborHigher ? "below" : "above"
    };
  }

  // 参考线文字选左端还是右端：比较两端最近数据点到参考线的屏幕竖直距离，选更远的一端。
  function chooseReferenceLabelSide(leftDistance, rightDistance) {
    var left = Number(leftDistance);
    var right = Number(rightDistance);
    if (!isFinite(left) && !isFinite(right)) return "left";
    if (!isFinite(left)) return "right";
    if (!isFinite(right)) return "left";
    return right > left ? "right" : "left";
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

  function rowNote(config, row) {
    if (row.note) return row.note;
    if (
      config.notes &&
      Object.prototype.hasOwnProperty.call(config.notes, row.label)
    ) {
      return config.notes[row.label];
    }
    return "";
  }

  function makeLegend(items) {
    var legend = element("div", "cu-chart-legend");
    for (var i = 0; i < items.length; i++) {
      var item = element("span", "cu-chart-legend-item");
      item.appendChild(
        element(
          "span",
          "cu-chart-legend-swatch " +
            items[i].className +
            (items[i].dashed ? " cu-legend-dashed" : "")
        )
      );
      item.appendChild(element("span", "", items[i].text));
      legend.appendChild(item);
    }
    return legend;
  }

  function makeRightControls() {
    var right = element("div", "cu-chart-right-controls");
    right.appendChild(element("span", "cu-chart-hint", hoverHint()));
    return right;
  }

  function buildColorScale(config, table, seriesMode) {
    if (seriesMode) {
      var seriesItems = [];
      for (var s = 0; s < config.series.length; s++) {
        seriesItems.push({
          name: config.series[s],
          className: SERIES_CLASSES[s]
        });
      }
      return { series: seriesItems, classes: {}, legend: seriesItems };
    }

    if (config.color === "single") {
      return {
        series: null,
        classes: {},
        legend: [],
        classFor: function () {
          return "cu-s1";
        }
      };
    }

    var classes = {};
    var order = [];
    for (var g = 0; g < table.groups.length; g++) {
      var rows = table.groups[g].rows;
      for (var r = 0; r < rows.length; r++) {
        var key;
        if (config.color === "rows") {
          key = config.rowGroups[rows[r].label];
          if (key === undefined) key = groupName(rows[r].label);
        } else {
          key = rows[r].group || groupName(rows[r].label);
        }
        if (!Object.prototype.hasOwnProperty.call(classes, key)) {
          // 只有 series 列表限制 4 个；first-word / rows 分组多了就循环用四种颜色。
          classes[key] = SERIES_CLASSES[order.length % SERIES_CLASSES.length];
          order.push(key);
        }
      }
    }

    return {
      series: null,
      classes: classes,
      legend: order.map(function (key) {
        return { text: key, className: classes[key] };
      }),
      classFor: function (row) {
        var key =
          config.color === "rows"
            ? config.rowGroups[row.label]
            : row.group || groupName(row.label);
        return classes[key] || "cu-s1";
      }
    };
  }

  function buildBarPanels(config, table, colorScale, seriesMode) {
    var panels = [];
    var groups = table.groups;
    var mode;

    if (config.panels === "groups") mode = "groups";
    else if (groups.length > 1 && config.value === undefined) mode = "groups";
    else mode = "columns";

    for (var g = 0; g < groups.length; g++) {
      var group = groups[g];
      var valueIndices = resolveValueIndices(config, group);
      if (seriesMode) valueIndices = [0];
      else if (!valueIndices) valueIndices = group.columns.map(function (_, i) {
        return i;
      });

      if (mode === "groups") {
        var indices = seriesMode ? [0] : valueIndices.slice(0, 1);
        if (config.value !== undefined && !seriesMode) {
          indices = valueIndices.slice();
        }
        for (var vi = 0; vi < indices.length; vi++) {
          panels.push(makeBarPanel(config, group, indices[vi], colorScale, seriesMode, true));
        }
      } else {
        for (var ci = 0; ci < valueIndices.length; ci++) {
          panels.push(
            makeBarPanel(config, group, valueIndices[ci], colorScale, seriesMode, false)
          );
        }
      }
    }

    if (!panels.length) throw new Error("没有可画的柱状图");
    return panels;
  }

  function makeBarPanel(config, group, valueIndex, colorScale, seriesMode, byGroup) {
    var series = null;
    var columnIndices = [valueIndex];
    var panelColumns = [group.columns[valueIndex]];

    if (seriesMode) {
      columnIndices = [];
      panelColumns = [];
      for (var s = 0; s < config.series.length; s++) {
        var idx = resolveColumnIndex(group, config.series[s]);
        columnIndices.push(idx);
        panelColumns.push({
          name: config.series[s],
          unit: group.columns[idx].unit
        });
      }
    }

    var rows = [];
    for (var r = 0; r < group.rows.length; r++) {
      var sourceRow = group.rows[r];
      var cells = [];
      for (var c = 0; c < columnIndices.length; c++) {
        var cell = sourceRow.values[columnIndices[c]];
        if (!cell) {
          throw new Error("行 " + sourceRow.label + " 缺少列 " + panelColumns[c].name);
        }
        cells.push(cell);
      }
      rows.push({
        label: sourceRow.label,
        group: sourceRow.group,
        groupClass: colorScale.classFor
          ? colorScale.classFor(sourceRow)
          : "cu-s1",
        values: cells,
        note: rowNote(config, sourceRow)
      });
    }

    if (!rows.length) throw new Error("分组没有数据行");

    var firstColumn = panelColumns[0];
    var title = "";
    if (seriesMode) {
      // 一组系列就是一幅图；标题只认显式的 panel，不拿第一个系列名充数。
      title = typeof config.panel === "string" ? config.panel : "";
    } else if (byGroup) {
      title = group.title || (firstColumn.auto ? "" : firstColumn.name);
    } else if (firstColumn.auto) {
      title = group.title || "";
    } else {
      title = group.title
        ? group.title + " · " + firstColumn.name
        : firstColumn.name;
    }

    var subtitle = "";
    if (config.panels && typeof config.panels === "object") {
      subtitle =
        (!firstColumn.auto && config.panels[firstColumn.name]) ||
        (byGroup && group.title ? config.panels[group.title] : "") ||
        "";
    }

    return {
      title: title,
      subtitle: subtitle,
      columnName: firstColumn.name,
      showColumnName:
        !seriesMode &&
        !byGroup &&
        config.value === undefined &&
        group.columns.length > 1 &&
        !firstColumn.auto,
      columns: panelColumns,
      rows: rows,
      unit: config.unit || firstColumn.unit || ""
    };
  }

  /* ------------------------------ 柱状图 ------------------------------ */

  function initBars(state) {
    var fig = state.fig;
    var plot = state.plot;
    var config = state.config;
    var data = state.data;

    var seriesMode = Array.isArray(config.series) && config.series.length > 0;
    var colorScale = buildColorScale(config, data, seriesMode);
    var panels = buildBarPanels(config, data, colorScale, seriesMode);
    var barRefs = Array.isArray(config.refs) ? config.refs : [];

    for (var p = 0; p < panels.length; p++) {
      var maxValue = 0;
      for (var r = 0; r < panels[p].rows.length; r++) {
        for (var c = 0; c < panels[p].rows[r].values.length; c++) {
          maxValue = Math.max(maxValue, panels[p].rows[r].values[c].value);
        }
        if (!seriesMode) {
          var panelSpread = findSpread(
            config,
            panels[p].rows[r].label,
            panels[p].columns[0].name
          );
          if (panelSpread) {
            maxValue = Math.max(
              maxValue,
              Number(panelSpread.lo),
              Number(panelSpread.hi)
            );
          }
        }
      }
      if (config.log) {
        if (maxValue <= 0) throw new Error("log 柱状图要求数值大于 0");
      }
      panels[p].max = maxValue;
    }

    if (!seriesMode && config.color !== "single") {
      makeHead(fig, plot, makeLegend(colorScale.legend), makeRightControls());
    } else if (seriesMode) {
      makeHead(
        fig,
        plot,
        makeLegend(colorScale.series.map(function (item) {
          return { text: item.name, className: item.className };
        })),
        makeRightControls()
      );
    } else {
      makeHead(fig, plot, element("span", "cu-chart-ylabel"), makeRightControls());
    }

    var hotNodes = [];
    var hits = [];

    function clearHot() {
      for (var i = 0; i < hotNodes.length; i++) {
        hotNodes[i].classList.remove("cu-hot");
      }
      hotNodes = [];
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
      hits = [];
      while (plot.firstChild) plot.removeChild(plot.firstChild);

      var family = getComputedStyle(fig).fontFamily || "sans-serif";
      var labelFont = 13;
      var valueFont = 12;
      var tickFont = 12;
      var barHeight = 18;
      var titleHeight = barRefs.length ? 40 : 26;
      var tickHeight = 24;
      var gap = 16;
      var topPadding = 4;
      var narrow = width < 420;
      var seriesCount = seriesMode ? config.series.length : 1;
      var rowHeight = seriesMode
        ? Math.max(30, seriesCount * 12 + (seriesCount - 1) * 2 + 8)
        : 30;
      if (seriesMode) barHeight = 12;

      var naturalLabelWidth = 0;
      for (var p = 0; p < panels.length; p++) {
        for (var r = 0; r < panels[p].rows.length; r++) {
          naturalLabelWidth = Math.max(
            naturalLabelWidth,
            textWidth(family, labelFont, 400, panels[p].rows[r].label)
          );
        }
      }
      var naturalWithGap = Math.ceil(naturalLabelWidth) + 4;
      var labelCap = narrow ? Math.floor(width * 0.45) : Infinity;
      var labelWidth = Math.min(naturalWithGap, labelCap);
      var wrapNeeded = labelWidth < naturalWithGap;
      var wrapWidth = Math.max(20, labelWidth - 4);

      var tickSets = [];
      var tickScales = [];
      var domains = [];
      var valueWidth = 0;
      var lastTickWidth = 0;

      for (var pp = 0; pp < panels.length; pp++) {
        var panel = panels[pp];
        var panelUnit = panel.unit;
        var max = panel.max;
        var values = [];
        for (var pr = 0; pr < panel.rows.length; pr++) {
          for (var pc = 0; pc < panel.rows[pr].values.length; pc++) {
            values.push(panel.rows[pr].values[pc].value);
          }
        }

        if (config.log) {
          var minPositive = Infinity;
          for (var lv = 0; lv < values.length; lv++) {
            if (values[lv] > 0) minPositive = Math.min(minPositive, values[lv]);
          }
          var lo = Math.pow(10, Math.floor(Math.log10(minPositive)));
          var hi = Math.pow(10, Math.ceil(Math.log10(max)));
          if (hi === lo) hi = lo * 10;
          domains[pp] = { lo: lo, hi: hi };
          tickSets[pp] = log10Ticks(lo, hi);
          if (!tickSets[pp].length) tickSets[pp] = [lo, hi];
          max = tickSets[pp][tickSets[pp].length - 1] || hi;
          panel.axisMax = max;
        } else {
          domains[pp] = { lo: 0, hi: max > 0 ? max : 1 };
          tickSets[pp] = niceTicks(max > 0 ? max : 1, 4);
          panel.axisMax = tickSets[pp][tickSets[pp].length - 1] || 1;
        }

        var tickScale = magnitudeScale(
          tickSets[pp].reduce(function (largest, tick) {
            return Math.max(largest, Math.abs(tick));
          }, 0)
        );
        tickScales[pp] = tickScale;
        for (var vr = 0; vr < panel.rows.length; vr++) {
          for (var vc = 0; vc < panel.rows[vr].values.length; vc++) {
            var cell = panel.rows[vr].values[vc];
            valueWidth = Math.max(
              valueWidth,
              textWidth(family, valueFont, 400, cell.text + " " + panelUnit)
            );
          }
        }
        lastTickWidth = Math.max(
          lastTickWidth,
          textWidth(
            family,
            tickFont,
            400,
            formatMagnitudeTick(panel.axisMax, tickScale) +
              (panelUnit ? " " + panelUnit : "")
          )
        );
      }

      var barsLeft = labelWidth + 12;
      var rightPadding = Math.ceil(
        Math.max(valueWidth + 12, lastTickWidth / 2 + 10, 16)
      );
      var plotWidth = Math.max(60, width - barsLeft - rightPadding);
      var svgWidth = barsLeft + plotWidth + rightPadding;

      var barsHeights = [];
      for (var bh = 0; bh < panels.length; bh++) {
        barsHeights[bh] = panels[bh].rows.length * rowHeight;
      }

      var panelHeights = [];
      for (var ph = 0; ph < panels.length; ph++) {
        panelHeights[ph] = titleHeight + barsHeights[ph] + tickHeight;
      }
      var svgHeight =
        topPadding +
        panelHeights.reduce(function (a, b) {
          return a + b;
        }, 0) +
        Math.max(0, panels.length - 1) * gap +
        8;

      var svg = svgElement("svg");
      svg.setAttribute("width", String(svgWidth));
      svg.setAttribute("height", String(svgHeight));
      svg.setAttribute("viewBox", "0 0 " + svgWidth + " " + svgHeight);
      svg.setAttribute("role", "group");
      svg.setAttribute("aria-label", captionText(fig));

      var panelTop = topPadding;

      for (var pi = 0; pi < panels.length; pi++) {
        var currentPanel = panels[pi];
        var panelUnit2 = currentPanel.unit;
        var barsTop = panelTop + titleHeight;
        var barsBottom = barsTop + barsHeights[pi];
        var max2 = currentPanel.axisMax;
        var ticks = tickSets[pi];
        var domain = domains[pi];

        if (currentPanel.title) {
          addText(svg, 0, panelTop + 15, currentPanel.title, "cu-panel-name");
          if (currentPanel.subtitle) {
            var nameWidth = textWidth(
              family,
              labelFont,
              600,
              currentPanel.title
            );
            addText(
              svg,
              nameWidth + 8,
              panelTop + 15,
              currentPanel.subtitle,
              "cu-panel-sub"
            );
          }
        }

        function xOf(value) {
          if (config.log) {
            var lr =
              (Math.log10(value) - Math.log10(domain.lo)) /
              (Math.log10(domain.hi) - Math.log10(domain.lo));
            if (lr < 0) lr = 0;
            if (lr > 1) lr = 1;
            return barsLeft + lr * plotWidth;
          }
          return barsLeft + (value / max2) * plotWidth;
        }

        for (var t = 0; t < ticks.length; t++) {
          var tickX = config.log
            ? xOf(ticks[t])
            : barsLeft + (ticks[t] / max2) * plotWidth;
          if (ticks[t] > 0) {
            addLine(svg, tickX, barsTop, tickX, barsBottom, "cu-grid");
          }
          var isLast = t === ticks.length - 1;
          var tickLabel =
            formatMagnitudeTick(ticks[t], tickScales[pi]) +
            (isLast && panelUnit2 ? " " + panelUnit2 : "");
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

        for (var rowIndex = 0; rowIndex < currentPanel.rows.length; rowIndex++) {
          var row = currentPanel.rows[rowIndex];
          var rowTop = barsTop + rowIndex * rowHeight;
          var centerY = rowTop + rowHeight / 2;
          var labelLines = wrapText(
            family,
            labelFont,
            400,
            row.label,
            wrapWidth,
            2
          );
          if (wrapNeeded && labelLines.length === 2) {
            addText(
              svg,
              labelWidth,
              centerY - 1,
              labelLines[0],
              "cu-label",
              "end"
            );
            addText(
              svg,
              labelWidth,
              centerY + 13,
              labelLines[1],
              "cu-label",
              "end"
            );
          } else {
            addText(svg, labelWidth, centerY + 4, row.label, "cu-label", "end");
          }

          var barInfos = [];
          var groupBarHeight = seriesMode ? barHeight : 18;
          var totalBarHeight =
            seriesMode
              ? seriesCount * groupBarHeight + (seriesCount - 1) * 2
              : groupBarHeight;
          var firstBarTop = centerY - totalBarHeight / 2;

          for (var si = 0; si < row.values.length; si++) {
            var cell2 = row.values[si];
            var barClass = seriesMode
              ? "cu-bar " + SERIES_CLASSES[si]
              : "cu-bar " + row.groupClass;
            var barEnd = config.log
              ? xOf(cell2.value)
              : barsLeft + (cell2.value / max2) * plotWidth;
            if (barEnd < barsLeft + 2) barEnd = barsLeft + 2;
            var top = seriesMode
              ? firstBarTop + si * (groupBarHeight + 2)
              : centerY - groupBarHeight / 2;
            var bottom = top + groupBarHeight;
            var bar = svgElement("path", barClass);
            bar.setAttribute(
              "d",
              barPath(barsLeft, barEnd, top, bottom, seriesMode ? 2 : 4)
            );
            svg.appendChild(bar);
            barInfos.push({
              bar: bar,
              end: barEnd,
              cell: cell2,
              centerY: (top + bottom) / 2
            });
          }

          var spread = null;
          if (!seriesMode) {
            spread = findSpread(
              config,
              row.label,
              currentPanel.columns[0].name
            );
          }
          var spreadEnd = 0;
          if (spread) {
            var lowX = config.log
              ? xOf(Number(spread.lo))
              : barsLeft + (Number(spread.lo) / max2) * plotWidth;
            var highX = config.log
              ? xOf(Number(spread.hi))
              : barsLeft + (Number(spread.hi) / max2) * plotWidth;
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

          var valueEnd = 0;
          for (var vi = 0; vi < barInfos.length; vi++) {
            var valueX = Math.max(barInfos[vi].end, spreadEnd) + 6;
            valueEnd = Math.max(valueEnd, valueX);
            addText(
              svg,
              valueX,
              barInfos[vi].centerY + 4,
              barInfos[vi].cell.text + (panelUnit2 ? " " + panelUnit2 : ""),
              "cu-value",
              "start"
            );
          }

          addBarHit({
            svg: svg,
            panel: currentPanel,
            row: row,
            barInfos: barInfos,
            unit: panelUnit2,
            config: config,
            x: barsLeft,
            barEnd: Math.max.apply(
              null,
              barInfos.map(function (item) {
                return item.end;
              })
            ),
            valueEnd: valueEnd,
            centerY: centerY,
            rowTop: rowTop,
            rowHeight: rowHeight,
            svgWidth: svgWidth,
            seriesMode: seriesMode,
            spread: spread
          });
        }

        for (var refIndex = 0; refIndex < barRefs.length; refIndex++) {
          var barRef = barRefs[refIndex] || {};
          var refValue = Number(barRef.x);
          if (!isFinite(refValue) || (config.log && refValue <= 0)) continue;
          var refX = config.log
            ? xOf(refValue)
            : barsLeft + (refValue / max2) * plotWidth;
          if (refX < barsLeft - 0.5 || refX > barsLeft + plotWidth + 0.5) {
            continue;
          }
          addLine(svg, refX, barsTop, refX, barsBottom, "cu-ref");
          var refText = String(barRef.label || "");
          if (!refText) continue;
          var refTextWidth = textWidth(family, 12, 400, refText);
          var refLabelX = refX;
          var refAnchor = "middle";
          if (refLabelX - refTextWidth / 2 < 0) {
            refLabelX = 0;
            refAnchor = "start";
          } else if (refLabelX + refTextWidth / 2 > svgWidth) {
            refLabelX = svgWidth;
            refAnchor = "end";
          }
          addText(
            svg,
            refLabelX,
            barsTop - 6,
            refText,
            "cu-ref-label",
            refAnchor
          );
        }

        panelTop += panelHeights[pi] + gap;
      }

      plot.appendChild(svg);
      if (config.xLabel) addXLabel(fig, plot, config.xLabel);
      else addXLabel(fig, plot, "");

      function addBarHit(info) {
        var hit = svgElement("rect", "cu-hit");
        hit.setAttribute("x", "0");
        hit.setAttribute("y", String(round(info.rowTop)));
        hit.setAttribute("width", String(round(info.svgWidth)));
        hit.setAttribute("height", String(info.rowHeight));
        hit.setAttribute("tabindex", "0");

        var unit = info.unit;
        var columnSuffix =
          info.panel.showColumnName && info.panel.columnName
            ? " · " + info.panel.columnName
            : "";
        var rowTitle = info.row.label + columnSuffix;
        var lines = [
          {
            keyClass: info.seriesMode ? "cu-axis" : info.row.groupClass,
            text: rowTitle
          }
        ];
        var aria = [rowTitle];

        if (info.seriesMode) {
          var baselineRowSeries = config.baseline
            ? findRowInPanel(info.panel, config.baseline)
            : null;
          for (var si = 0; si < info.row.values.length; si++) {
            var seriesCell = info.row.values[si];
            var seriesName = info.panel.columns[si].name;
            var valueText =
              seriesCell.text + (unit ? " " + unit : "");
            lines.push({
              keyClass: SERIES_CLASSES[si],
              name: seriesName,
              value: valueText
            });
            aria.push(seriesName + " " + valueText);

            if (baselineRowSeries && info.row.label !== config.baseline) {
              var baselineCell = baselineRowSeries.values[si];
              if (baselineCell && seriesCell.value !== 0) {
                var speedup =
                  config.better === "higher"
                    ? seriesCell.value / baselineCell.value
                    : baselineCell.value / seriesCell.value;
                var speedupText = formatSpeedup(speedup);
                lines.push({
                  className: "cu-tip-meta",
                  text:
                    seriesName +
                    " 相对 " +
                    config.baseline +
                    " 的加速比 " +
                    speedupText
                });
                aria.push(seriesName + " 加速比 " + speedupText);
              }
            }

            if (config.share) {
              var total = 0;
              for (var sr = 0; sr < info.panel.rows.length; sr++) {
                total += info.panel.rows[sr].values[si].value;
              }
              var ratio = total > 0 ? (seriesCell.value / total) * 100 : 0;
              var shareText =
                ratio < 0.1 ? "不到 0.1%" : ratio.toFixed(1) + "%";
              lines.push({
                className: "cu-tip-meta",
                text: seriesName + " 占总时间的 " + shareText
              });
              aria.push(seriesName + " 占总时间的 " + shareText);
            }
          }
        } else {
          var cell = info.row.values[0];
          var valueText2 = cell.text + (unit ? " " + unit : "");
          lines.push({ className: "cu-tip-value", text: valueText2 });
          aria.push(valueText2);

          if (config.baseline) {
            if (info.row.label === config.baseline) {
              lines.push({ className: "cu-tip-meta", text: "基准" });
              aria.push("基准");
            } else {
              var baselineRow = findRowInPanel(info.panel, config.baseline);
              var baselineCell2 = baselineRow && baselineRow.values[0];
              if (baselineCell2 && cell.value !== 0) {
                var speedup2 =
                  config.better === "higher"
                    ? cell.value / baselineCell2.value
                    : baselineCell2.value / cell.value;
                var speedupText2 = formatSpeedup(speedup2);
                lines.push({
                  className: "cu-tip-meta",
                  text:
                    "相对 " +
                    config.baseline +
                    "的加速比 " +
                    speedupText2
                });
                aria.push(
                  "相对 " + config.baseline + "的加速比 " + speedupText2
                );
              }
            }
          }

          if (config.share) {
            var total2 = 0;
            for (var sr2 = 0; sr2 < info.panel.rows.length; sr2++) {
              total2 += info.panel.rows[sr2].values[0].value;
            }
            var ratio2 = total2 > 0 ? (cell.value / total2) * 100 : 0;
            var shareText2 =
              ratio2 < 0.1 ? "不到 0.1%" : ratio2.toFixed(1) + "%";
            lines.push({
              className: "cu-tip-meta",
              text: "占总时间的 " + shareText2
            });
            aria.push("占总时间的 " + shareText2);
          }
        }

        if (info.row.note) {
          lines.push({ className: "cu-tip-note", text: info.row.note });
          aria.push(info.row.note);
        }

        if (info.spread && info.spread.note) {
          lines.push({ className: "cu-tip-note", text: info.spread.note });
          aria.push(info.spread.note);
        }

        hit.setAttribute("aria-label", aria.join("，"));

        var hitIndex = hits.length;
        hits.push(hit);

        function showPointer(event) {
          var point = figurePoint(fig, event.clientX, event.clientY);
          showRow(point.x, point.y);
        }

        function showRow(x, y) {
          clearHot();
          hotNodes = [];
          for (var b = 0; b < info.barInfos.length; b++) {
            info.barInfos[b].bar.classList.add("cu-hot");
            hotNodes.push(info.barInfos[b].bar);
          }
          hit.classList.add("cu-hot");
          hotNodes.push(hit);
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
        hit.addEventListener("focus", showAtBar);
        hit.addEventListener("blur", clearRow);
        hit.addEventListener("keydown", function (event) {
          var key = event.key;
          if (
            key === "ArrowDown" ||
            key === "ArrowRight" ||
            key === "ArrowUp" ||
            key === "ArrowLeft" ||
            key === "Home" ||
            key === "End"
          ) {
            // mdBook 用左右方向键翻章，焦点在图上时不能让它收到。
            event.preventDefault();
            event.stopPropagation();
            var next = hitIndex;
            if (key === "Home") next = 0;
            else if (key === "End") next = hits.length - 1;
            else if (key === "ArrowDown" || key === "ArrowRight") {
              next = Math.min(hits.length - 1, hitIndex + 1);
            } else {
              next = Math.max(0, hitIndex - 1);
            }
            if (hits[next]) hits[next].focus();
          } else if (key === "Escape") {
            clearRow();
          }
        });

        svg.appendChild(hit);
      }
    };
  }

  function findRowInPanel(panel, label) {
    for (var i = 0; i < panel.rows.length; i++) {
      if (panel.rows[i].label === label) return panel.rows[i];
    }
    return null;
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

  /* ------------------------------ 天平图 ------------------------------ */

  function balanceSegmentPath(x0, x1, top, bottom, roundLeft, roundRight) {
    var width = x1 - x0;
    if (width <= 0.01) return "";
    var radius = Math.min(4, width, (bottom - top) / 2);
    var leftRadius = roundLeft ? radius : 0;
    var rightRadius = roundRight ? radius : 0;

    var d =
      "M " +
      round(x0 + leftRadius) +
      " " +
      round(top) +
      " H " +
      round(x1 - rightRadius);
    if (rightRadius >= 0.5) {
      d +=
        " Q " +
        round(x1) +
        " " +
        round(top) +
        " " +
        round(x1) +
        " " +
        round(top + rightRadius);
    }
    d += " V " + round(bottom - rightRadius);
    if (rightRadius >= 0.5) {
      d +=
        " Q " +
        round(x1) +
        " " +
        round(bottom) +
        " " +
        round(x1 - rightRadius) +
        " " +
        round(bottom);
    }
    d += " H " + round(x0 + leftRadius);
    if (leftRadius >= 0.5) {
      d +=
        " Q " +
        round(x0) +
        " " +
        round(bottom) +
        " " +
        round(x0) +
        " " +
        round(bottom - leftRadius);
    }
    d += " V " + round(top + leftRadius);
    if (leftRadius >= 0.5) {
      d +=
        " Q " +
        round(x0) +
        " " +
        round(top) +
        " " +
        round(x0 + leftRadius) +
        " " +
        round(top);
    }
    return d + " Z";
  }

  function initBalance(state) {
    var fig = state.fig;
    var plot = state.plot;
    var config = state.config;
    var rowsConfig = Array.isArray(config.rows) ? config.rows : [];
    var tasks = Array.isArray(config.tasks) ? config.tasks : [];

    var domain = config.domain;
    var domainLo = Number(domain[0]);
    var domainHi = Number(domain[1]);

    var leftText = config.left || "受带宽限制";
    var rightText = config.right || "受算力限制";
    var unit = config.unit || "";

    var legend = makeLegend([
      { className: "cu-s1", text: leftText },
      { className: "cu-s2", text: rightText }
    ]);

    makeHead(fig, plot, legend, makeRightControls());

    var hot = null;

    function clearHot() {
      if (!hot) return;
      for (var i = 0; i < hot.segments.length; i++) {
        if (hot.segments[i]) hot.segments[i].classList.remove("cu-hot");
      }
      if (hot.hit) hot.hit.classList.remove("cu-hot");
      hot = null;
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
      var noteFont = 12;
      var taskFont = 12;
      var rowHeight = 46;
      var trackHeight = 14;
      var trackOffset = (rowHeight - trackHeight) / 2;
      var topPadding = 38;
      var axisHeight = 32;
      var labelGap = 12;
      var rightPadding = 8;

      var naturalLabelWidth = 0;
      for (var r = 0; r < rowsConfig.length; r++) {
        var configRow = rowsConfig[r];
        naturalLabelWidth = Math.max(
          naturalLabelWidth,
          textWidth(family, labelFont, 400, configRow.label || ""),
          textWidth(family, noteFont, 400, configRow.note || "")
        );
      }
      naturalLabelWidth = Math.ceil(naturalLabelWidth) + 4;

      var maxLabelWidth = Math.max(110, Math.round(width * 0.4));
      var labelWidth = Math.max(
        110,
        Math.min(naturalLabelWidth, maxLabelWidth)
      );
      var trackLeft = labelWidth + labelGap;
      var trackWidth = Math.max(60, width - trackLeft - rightPadding);
      var trackRight = trackLeft + trackWidth;
      var svgWidth = trackRight + rightPadding;
      var firstTrackTop = topPadding + trackOffset;
      var lastTrackBottom =
        topPadding +
        (rowsConfig.length - 1) * rowHeight +
        trackOffset +
        trackHeight;
      var svgHeight = topPadding + rowsConfig.length * rowHeight + axisHeight;
      var axisY = lastTrackBottom + 10;

      function xOf(value) {
        var numeric = Number(value);
        if (!isFinite(numeric) || numeric <= 0) return trackLeft;
        var ratio =
          (Math.log10(numeric) - Math.log10(domainLo)) /
          (Math.log10(domainHi) - Math.log10(domainLo));
        if (ratio < 0) ratio = 0;
        if (ratio > 1) ratio = 1;
        return trackLeft + ratio * trackWidth;
      }

      var svg = svgElement("svg");
      svg.setAttribute("width", String(svgWidth));
      svg.setAttribute("height", String(svgHeight));
      svg.setAttribute("viewBox", "0 0 " + svgWidth + " " + svgHeight);
      svg.setAttribute("role", "group");
      svg.setAttribute("aria-label", captionText(fig));

      var ticks = log10Ticks(domainLo, domainHi);
      for (var t = 0; t < ticks.length; t++) {
        var gridX = xOf(ticks[t]);
        addLine(svg, gridX, firstTrackTop, gridX, lastTrackBottom, "cu-grid");
        addText(svg, gridX, axisY + 17, formatTick(ticks[t]), "cu-tick", "middle");
      }

      addLine(svg, trackLeft, axisY, trackRight, axisY, "cu-axis");

      var rowInfo = [];
      for (var rowIndex = 0; rowIndex < rowsConfig.length; rowIndex++) {
        var row = rowsConfig[rowIndex];
        var balanceValue = Number(row.balance);
        if (!isFinite(balanceValue)) {
          throw new Error("balance 值无效：" + (row.label || rowIndex + 1));
        }

        var rowTop = topPadding + rowIndex * rowHeight;
        var centerY = rowTop + trackOffset + trackHeight / 2;
        var top = rowTop + trackOffset;
        var bottom = top + trackHeight;
        var balanceX = xOf(balanceValue);
        if (balanceX < trackLeft) balanceX = trackLeft;
        if (balanceX > trackRight) balanceX = trackRight;

        var leftSegment = null;
        var rightSegment = null;
        if (balanceX - 1 > trackLeft + 0.01) {
          leftSegment = svgElement("path", "cu-balance-seg cu-s1");
          leftSegment.setAttribute(
            "d",
            balanceSegmentPath(trackLeft, balanceX - 1, top, bottom, true, false)
          );
          svg.appendChild(leftSegment);
        }
        if (balanceX + 1 < trackRight - 0.01) {
          rightSegment = svgElement("path", "cu-balance-seg cu-s2");
          rightSegment.setAttribute(
            "d",
            balanceSegmentPath(balanceX + 1, trackRight, top, bottom, false, true)
          );
          svg.appendChild(rightSegment);
        }

        var rowLabelText = truncateText(
          family,
          labelFont,
          400,
          row.label || "",
          labelWidth
        );
        var noteText = truncateText(
          family,
          noteFont,
          400,
          row.note || "",
          labelWidth
        );
        addText(svg, labelWidth, centerY - 3, rowLabelText, "cu-label", "end");
        if (noteText) {
          addText(svg, labelWidth, centerY + 12, noteText, "cu-note", "end");
        }

        var balanceText = "平衡点 " + String(row.balance);
        var balanceWidth = textWidth(family, 12, 400, balanceText);
        var balanceLabelX = balanceX;
        var balanceAnchor = "middle";
        if (balanceLabelX - balanceWidth / 2 < trackLeft) {
          balanceLabelX = trackLeft;
          balanceAnchor = "start";
        } else if (balanceLabelX + balanceWidth / 2 > trackRight) {
          balanceLabelX = trackRight;
          balanceAnchor = "end";
        }

        rowInfo.push({
          row: row,
          rowIndex: rowIndex,
          rowTop: rowTop,
          centerY: centerY,
          top: top,
          balanceValue: balanceValue,
          balanceX: balanceX,
          leftSegment: leftSegment,
          rightSegment: rightSegment,
          balanceText: balanceText,
          balanceLabelX: balanceLabelX,
          balanceAnchor: balanceAnchor
        });
      }

      var lineTop = firstTrackTop - 14;

      for (var taskIndex = 0; taskIndex < tasks.length; taskIndex++) {
        var taskX = xOf(tasks[taskIndex].x);
        addLine(svg, taskX, lineTop, taskX, lastTrackBottom, "cu-task-line");
      }

      for (var dotTask = 0; dotTask < tasks.length; dotTask++) {
        var dotX = xOf(tasks[dotTask].x);
        for (var dotRow = 0; dotRow < rowInfo.length; dotRow++) {
          var dotSide =
            dotX < rowInfo[dotRow].balanceX ? "cu-s1" : "cu-s2";
          var dot = svgElement("circle", "cu-task-dot " + dotSide);
          dot.setAttribute("cx", String(round(dotX)));
          dot.setAttribute("cy", String(round(rowInfo[dotRow].centerY)));
          dot.setAttribute("r", "5");
          svg.appendChild(dot);
        }
      }

      for (var labelRow = 0; labelRow < rowInfo.length; labelRow++) {
        addText(
          svg,
          rowInfo[labelRow].balanceLabelX,
          rowInfo[labelRow].top - 6,
          rowInfo[labelRow].balanceText,
          "cu-balance-label",
          rowInfo[labelRow].balanceAnchor
        );
      }

      var baseTaskY = Math.max(12, lineTop - 26);
      var taskLabels = [];
      for (var taskLabelIndex = 0; taskLabelIndex < tasks.length; taskLabelIndex++) {
        var task = tasks[taskLabelIndex];
        var taskText =
          (task.label || "") +
          (task.text !== undefined && task.text !== null
            ? " " + task.text
            : "");
        var displayText = truncateText(
          family,
          taskFont,
          600,
          taskText,
          trackWidth
        );
        var textW = textWidth(family, taskFont, 600, displayText);
        var taskCenter = xOf(task.x);
        var labelX = taskCenter;
        var anchor = "middle";
        var labelLeft = taskCenter - textW / 2;
        var labelRight = taskCenter + textW / 2;

        if (labelLeft < trackLeft) {
          labelLeft = trackLeft;
          labelRight = labelLeft + textW;
          labelX = trackLeft;
          anchor = "start";
        } else if (labelRight > trackRight) {
          labelRight = trackRight;
          labelLeft = labelRight - textW;
          labelX = trackRight;
          anchor = "end";
        }

        taskLabels.push({
          text: displayText,
          x: labelX,
          anchor: anchor,
          left: labelLeft,
          right: labelRight,
          sortX: taskCenter,
          baseline: baseTaskY
        });
      }

      taskLabels.sort(function (a, b) {
        return a.sortX - b.sortX;
      });
      var rightByBaseline = {};
      for (var place = 0; place < taskLabels.length; place++) {
        var meta = taskLabels[place];
        var baseline = baseTaskY;
        var guard = 0;
        while (
          meta.left <
            (Object.prototype.hasOwnProperty.call(rightByBaseline, baseline)
              ? rightByBaseline[baseline]
              : -Infinity) +
              6 &&
          guard < 12
        ) {
          baseline += 16;
          guard++;
        }
        meta.baseline = baseline;
        rightByBaseline[baseline] = Math.max(
          Object.prototype.hasOwnProperty.call(rightByBaseline, baseline)
            ? rightByBaseline[baseline]
            : -Infinity,
          meta.right
        );
      }
      for (var drawLabel = 0; drawLabel < taskLabels.length; drawLabel++) {
        addText(
          svg,
          taskLabels[drawLabel].x,
          taskLabels[drawLabel].baseline,
          taskLabels[drawLabel].text,
          "cu-task-label",
          taskLabels[drawLabel].anchor
        );
      }

      var hits = [];

      for (var hitRow = 0; hitRow < rowInfo.length; hitRow++) {
        addBalanceHit(rowInfo[hitRow]);
      }

      function addBalanceHit(info) {
        var hit = svgElement("rect", "cu-hit");
        hit.setAttribute("x", "0");
        hit.setAttribute("y", String(round(info.rowTop)));
        hit.setAttribute("width", String(round(svgWidth)));
        hit.setAttribute("height", String(rowHeight));
        hit.setAttribute("tabindex", "0");

        var balanceLine =
          info.balanceText + (unit ? " " + unit : "");
        var lines = [
          {
            keyClass: "cu-axis",
            className: "cu-tip-meta",
            text: info.row.label
          },
          { className: "cu-tip-value", text: balanceLine }
        ];
        var ariaParts = [info.row.label, balanceLine];

        if (info.row.note) {
          lines.push({ className: "cu-tip-note", text: info.row.note });
          ariaParts.push(info.row.note);
        }

        for (var ti = 0; ti < tasks.length; ti++) {
          var taskLine =
            (tasks[ti].label || "") +
            (tasks[ti].text !== undefined && tasks[ti].text !== null
              ? " " + tasks[ti].text
              : "");
          var taskSide =
            Number(tasks[ti].x) < info.balanceValue ? leftText : rightText;
          var taskText = taskLine + "：" + taskSide;
          lines.push({
            keyClass:
              "cu-tip-square " +
              (Number(tasks[ti].x) < info.balanceValue ? "cu-s1" : "cu-s2"),
            className: "cu-tip-meta",
            text: taskText
          });
          ariaParts.push(taskText);
        }

        hit.setAttribute("aria-label", ariaParts.join("，"));

        var hitIndex = hits.length;
        hits.push(hit);

        function showRow(x, y) {
          clearHot();
          hot = {
            hit: hit,
            segments: [info.leftSegment, info.rightSegment]
          };
          if (info.leftSegment) info.leftSegment.classList.add("cu-hot");
          if (info.rightSegment) info.rightSegment.classList.add("cu-hot");
          hit.classList.add("cu-hot");
          showTip(state, lines, x, y);
        }

        function showAtBalance() {
          var point = plotPoint(fig, plot, info.balanceX, info.centerY);
          showRow(point.x, point.y);
        }

        function clearRow() {
          clearHot();
          hideTip(state);
        }

        hit.addEventListener("pointerenter", function (event) {
          if (event.pointerType === "touch") return;
          var point = figurePoint(fig, event.clientX, event.clientY);
          showRow(point.x, point.y);
        });
        hit.addEventListener("pointermove", function (event) {
          var point = figurePoint(fig, event.clientX, event.clientY);
          showRow(point.x, point.y);
        });
        hit.addEventListener("pointerleave", function () {
          if (document.activeElement !== hit) clearRow();
        });
        hit.addEventListener("pointerdown", function (event) {
          var point = figurePoint(fig, event.clientX, event.clientY);
          showRow(point.x, point.y);
        });
        hit.addEventListener("focus", showAtBalance);
        hit.addEventListener("blur", clearRow);
        hit.addEventListener("keydown", function (event) {
          var key = event.key;
          if (
            key === "ArrowDown" ||
            key === "ArrowRight" ||
            key === "ArrowUp" ||
            key === "ArrowLeft" ||
            key === "Home" ||
            key === "End"
          ) {
            // mdBook 用左右方向键翻章，焦点在图上时不能让它收到。
            event.preventDefault();
            event.stopPropagation();
            var next = hitIndex;
            if (key === "Home") next = 0;
            else if (key === "End") next = hits.length - 1;
            else if (key === "ArrowDown" || key === "ArrowRight") {
              next = Math.min(hits.length - 1, hitIndex + 1);
            } else {
              next = Math.max(0, hitIndex - 1);
            }
            if (next !== hitIndex && hits[next]) hits[next].focus();
          } else if (key === "Escape") {
            clearRow();
          }
        });

        svg.appendChild(hit);
      }

      plot.appendChild(svg);
      addXLabel(fig, plot, config.xLabel || "");
    };
  }

  /* ------------------------------ 折线图 ------------------------------ */

  function buildLinePanels(config, table) {
    var groups = table.groups;
    var seriesNames =
      Array.isArray(config.series) && config.series.length
        ? config.series
        : null;
    var extras = Array.isArray(config.extraSeries) ? config.extraSeries : [];
    var panels = [];
    var rowOffset = 0;

    for (var g = 0; g < groups.length; g++) {
      var group = groups[g];
      var names = [];
      var indices = [];

      if (seriesNames) {
        for (var s = 0; s < seriesNames.length; s++) {
          names.push(seriesNames[s]);
          indices.push(resolveColumnIndex(group, seriesNames[s]));
        }
      } else {
        names.push(group.columns[0].name);
        indices.push(0);
      }

      // 先把这一组数据行的 x 算出来；extraSeries 和普通系列共用。
      var rowXs = [];
      var rowXTexts = [];
      for (var xr = 0; xr < group.rows.length; xr++) {
        var xSourceRow = group.rows[xr];
        var xRaw;
        if (config.x !== undefined) {
          if (config.x === group.labelName) {
            xRaw = xSourceRow.label;
          } else {
            var xi = resolveColumnIndex(group, config.x);
            var xCell = xSourceRow.values[xi];
            if (isMissingCell(xCell)) {
              throw new Error("行 " + xSourceRow.label + " 的 x 列是缺值");
            }
            xRaw = xCell.text + (xCell.unit ? " " + xCell.unit : "");
          }
        } else {
          xRaw = xSourceRow.label;
        }
        rowXs.push(parseXValue(xRaw));
        rowXTexts.push(String(xRaw).trim());
      }

      var allSeries = [];
      var seriesAuto = [];
      for (var si = 0; si < indices.length; si++) {
        var points = [];
        seriesAuto.push(
          seriesNames
            ? false
            : !!(
                group.columns[indices[si]] &&
                group.columns[indices[si]].auto
              )
        );
        for (var r = 0; r < group.rows.length; r++) {
          var sourceRow = group.rows[r];
          var cell = sourceRow.values[indices[si]];
          if (cell === undefined) {
            throw new Error("行 " + sourceRow.label + " 缺少列 " + names[si]);
          }
          points.push({
            x: rowXs[r],
            xText: rowXTexts[r],
            cell: cell,
            label: sourceRow.label,
            note:
              sourceRow.note ||
              (config.notes && config.notes[sourceRow.label]) ||
              ""
          });
        }
        if (!splitLineSegments(points).length) {
          throw new Error(
            "系列 " + names[si] + " 全是缺值；折线图每条线至少要有一个有值的点"
          );
        }
        allSeries.push({
          name: names[si],
          auto: seriesAuto[si],
          className: SERIES_CLASSES[si % SERIES_CLASSES.length],
          points: points
        });
      }

      for (var e = 0; e < extras.length; e++) {
        var extra = extras[e];
        var extraPoints = [];
        for (var er = 0; er < group.rows.length; er++) {
          var extraRow = group.rows[er];
          var rawValue = extra.values[rowOffset + er];
          var numeric = Number(rawValue);
          extraPoints.push({
            x: rowXs[er],
            xText: rowXTexts[er],
            cell: { value: numeric, text: String(rawValue) },
            label: extraRow.label,
            note:
              extraRow.note ||
              (config.notes && config.notes[extraRow.label]) ||
              "",
            extra: true
          });
        }
        allSeries.push({
          name: extra.name,
          auto: false,
          className:
            SERIES_CLASSES[
              (indices.length + e) % SERIES_CLASSES.length
            ],
          dashed: !!extra.dashed,
          extra: true,
          points: extraPoints
        });
      }
      rowOffset += group.rows.length;

      var panelXName = "";
      if (typeof config.xName === "string" && config.xName.trim()) {
        panelXName = config.xName.trim();
      } else if (group.labelName) {
        panelXName = group.labelName;
      }

      panels.push({
        title: group.title || null,
        labelName: group.labelName || "",
        xName: panelXName,
        unit: config.unit || (group.columns[indices[0]] && group.columns[indices[0]].unit) || "",
        series: allSeries,
        xs: rowXs
      });
    }

    if (!panels.length) throw new Error("折线图没有数据");
    return panels;
  }

  function lineTicksFor(xScale, xMin, xMax, xFormat) {
    var ticks = [];
    if (xMax <= xMin) return [xMin];
    if (xScale === "linear") {
      if (xFormat === "bytes") {
        return byteLinearTicks(xMin, xMax, 5);
      }
      var rawStep = (xMax - xMin) / 5;
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
      var first = Math.ceil(xMin / step - 1e-9) * step;
      for (var v = first; v <= xMax + 1e-9; v += step) {
        ticks.push(Number(v.toPrecision(12)));
      }
      if (!ticks.length) ticks = [xMin, xMax];
      return ticks;
    }
    if (xScale === "log10") {
      var first10 = Math.ceil(Math.log10(xMin) - 1e-12);
      var last10 = Math.floor(Math.log10(xMax) + 1e-12);
      for (var p10 = first10; p10 <= last10; p10++) {
        ticks.push(Number(Math.pow(10, p10).toPrecision(12)));
      }
      if (!ticks.length) ticks = [xMin, xMax];
      return ticks;
    }
    var first2 = Math.ceil(Math.log2(xMin) - 1e-12);
    var last2 = Math.floor(Math.log2(xMax) + 1e-12);
    for (var p2 = first2; p2 <= last2; p2++) {
      ticks.push(Number(Math.pow(2, p2).toPrecision(12)));
    }
    if (!ticks.length) ticks = [xMin, xMax];
    return ticks;
  }

  function initLine(state) {
    var fig = state.fig;
    var plot = state.plot;
    var config = state.config;
    var data = state.data;

    var panels = buildLinePanels(config, data);
    var xScale = config.xScale || "log2";
    var xFormat = config.xFormat || "count";
    var allXs = [];
    for (var p = 0; p < panels.length; p++) {
      allXs = allXs.concat(panels[p].xs);
    }
    allXs.sort(function (a, b) {
      return a - b;
    });
    var xs = [];
    for (var x = 0; x < allXs.length; x++) {
      if (!xs.length || allXs[x] !== xs[xs.length - 1]) xs.push(allXs[x]);
    }
    if (!xs.length) throw new Error("折线图没有数据点");

    var xMin = xs[0];
    var xMax = xs[xs.length - 1];
    if (xScale !== "linear" && (xMin <= 0 || xMax <= 0)) {
      throw new Error("对数横轴要求 x 大于 0");
    }
    if (config.share && config.share.of !== undefined) {
      var shareOf = Number(config.share.of);
      if (!isFinite(shareOf) || shareOf <= 0) {
        throw new Error("share.of 必须是正数");
      }
    }

    var firstPanel = panels[0];
    var multiSeries = firstPanel.series.length > 1;
    var yLabel =
      config.yLabel ||
      (firstPanel.series[0].auto ? "" : firstPanel.series[0].name);
    var left = element("div", "cu-chart-head-meta");
    if (yLabel) {
      left.appendChild(element("span", "cu-chart-ylabel", yLabel));
    }
    if (multiSeries) {
      var legendItems = firstPanel.series.map(function (series) {
        return {
          text: series.name,
          className: series.className,
          dashed: !!series.dashed
        };
      });
      left.appendChild(makeLegend(legendItems));
    }

    function xTooltipText(index) {
      var name = firstPanel.xName || "";
      return (
        (name ? name + " " : "") + formatXValue(xs[index], xFormat)
      );
    }

    var right = makeRightControls();
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

    state.log = config.yScale === "log";
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
      try {
        state.render(state.width);
      } catch (error) {
        state.failed = true;
        failFigure(fig, plot, error);
      }
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
      var multiplePanels = panels.length > 1;
      var plotHeight = multiplePanels ? 190 : 280;
      var panelTitleHeight = multiplePanels ? 24 : 0;
      var marginLeft = 46;
      var marginRight = 18;
      var marginTop = 16 + panelTitleHeight;
      var marginBottom = 34;
      var plotWidth = Math.max(40, width - marginLeft - marginRight);
      var svgWidth = marginLeft + plotWidth + marginRight;
      var svgHeight = marginTop + plotHeight + marginBottom;

      var ticks = lineTicksFor(xScale, xMin, xMax, xFormat);
      var labelEvery =
        xScale === "linear" && xFormat === "bytes"
          ? 1
          : plotWidth < 420
            ? 3
            : 2;

      var plotLeftInner = marginLeft + 8;
      var plotRightInner = marginLeft + plotWidth - 8;

      function sx(value) {
        if (xMax === xMin) return marginLeft + plotWidth / 2;
        if (xScale === "linear") {
          var lr = (value - xMin) / (xMax - xMin);
          return plotLeftInner + lr * (plotRightInner - plotLeftInner);
        }
        var ratio =
          (Math.log(value) - Math.log(xMin)) /
          (Math.log(xMax) - Math.log(xMin));
        return plotLeftInner + ratio * (plotRightInner - plotLeftInner);
      }

      var panelYMax = [];
      var panelYTicks = [];
      var panelYMin = [];
      for (var pi = 0; pi < panels.length; pi++) {
        var values = [];
        for (var si = 0; si < panels[pi].series.length; si++) {
          for (var pt = 0; pt < panels[pi].series[si].points.length; pt++) {
            var pointCell = panels[pi].series[si].points[pt].cell;
            if (!isMissingCell(pointCell)) values.push(pointCell.value);
          }
        }
        if (!values.length) {
          throw new Error("折线图没有有值的点");
        }
        var dataMin = Math.min.apply(null, values);
        var dataMax = Math.max.apply(null, values);
        var refs = config.refs || [];
        var refMax = 0;
        for (var refIndex = 0; refIndex < refs.length; refIndex++) {
          refMax = Math.max(refMax, Number(refs[refIndex].y));
        }

        if (state.log) {
          if (dataMin <= 0) {
            throw new Error("对数纵轴要求数值大于 0");
          }
          var minPower = Math.floor(Math.log10(dataMin));
          var maxPower = Math.ceil(Math.log10(Math.max(dataMax, refMax)));
          panelYMin[pi] = Math.pow(10, minPower);
          panelYMax[pi] = Math.pow(10, maxPower);
          if (panelYMax[pi] === panelYMin[pi]) {
            panelYMax[pi] = panelYMin[pi] * 10;
          }
          panelYTicks[pi] = [];
          for (var power = minPower; power <= maxPower; power++) {
            panelYTicks[pi].push(
              Number(Math.pow(10, power).toPrecision(12))
            );
          }
          if (
            panelYTicks[pi][panelYTicks[pi].length - 1] !== panelYMax[pi]
          ) {
            panelYTicks[pi].push(panelYMax[pi]);
          }
        } else {
          var domainMax =
            config.yMax !== undefined && config.yMax !== null
              ? Number(config.yMax)
              : Math.max(dataMax, refMax);
          panelYTicks[pi] = niceTicks(domainMax, 4);
          panelYMin[pi] = 0;
          panelYMax[pi] = panelYTicks[pi][panelYTicks[pi].length - 1];
          if (!panelYMax[pi]) {
            panelYTicks[pi] = [0, 1];
            panelYMax[pi] = 1;
          }
        }
      }

      var svgList = [];
      var panelMeta = [];

      function makeSy(ymin, ymax) {
        return function (value) {
          if (state.log) {
            var lr =
              (Math.log10(value) - Math.log10(ymin)) /
              (Math.log10(ymax) - Math.log10(ymin));
            return marginTop + plotHeight - lr * plotHeight;
          }
          return marginTop + plotHeight - (value / ymax) * plotHeight;
        };
      }

      function endpointDistance(panelInfo, syFn, targetX, targetY) {
        var nearestX = null;
        for (var e1 = 0; e1 < panelInfo.series.length; e1++) {
          var ep = panelInfo.series[e1].points;
          for (var e2 = 0; e2 < ep.length; e2++) {
            if (isMissingCell(ep[e2].cell)) continue;
            if (
              nearestX === null ||
              Math.abs(ep[e2].x - targetX) < Math.abs(nearestX - targetX)
            ) {
              nearestX = ep[e2].x;
            }
          }
        }
        if (nearestX === null) return Infinity;
        var best = Infinity;
        for (var e3 = 0; e3 < panelInfo.series.length; e3++) {
          var points = panelInfo.series[e3].points;
          for (var e4 = 0; e4 < points.length; e4++) {
            if (isMissingCell(points[e4].cell)) continue;
            if (Math.abs(points[e4].x - nearestX) > 1e-9) continue;
            var distance = Math.abs(syFn(points[e4].cell.value) - targetY);
            if (distance < best) best = distance;
          }
        }
        return best;
      }

      function addDirectLabel(svgNode, refsMeta, text, x, y, anchor, className, weight) {
        var box = textBox(family, 12, weight || 400, text, x, y, anchor);
        for (var rm = 0; rm < refsMeta.length; rm++) {
          if (boxesIntersect(box, refsMeta[rm].box)) return null;
        }
        return addText(svgNode, x, y, text, className, anchor);
      }

      for (var pn = 0; pn < panels.length; pn++) {
        var panel = panels[pn];
        var yMin = panelYMin[pn];
        var yMax = panelYMax[pn];
        var sy = makeSy(yMin, yMax);

        var svg = svgElement("svg");
        svg.setAttribute("width", String(svgWidth));
        svg.setAttribute("height", String(svgHeight));
        svg.setAttribute("viewBox", "0 0 " + svgWidth + " " + svgHeight);
        svg.setAttribute("role", "group");
        svg.setAttribute(
          "aria-label",
          captionText(fig) + (panel.title ? "：" + panel.title : "")
        );

        if (multiplePanels && panel.title) {
          addText(svg, marginLeft, 16, panel.title, "cu-panel-name");
        }

        var labelIndex = 0;
        for (var t = 0; t < ticks.length; t++) {
          var gridX = sx(ticks[t]);
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
              formatAxisValue(ticks[t], xFormat),
              "cu-tick",
              "middle"
            );
          }
          labelIndex++;
        }

        for (var yt = 0; yt < panelYTicks[pn].length; yt++) {
          var tickY = sy(panelYTicks[pn][yt]);
          if (panelYTicks[pn][yt] !== 0) {
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
            formatTick(panelYTicks[pn][yt]),
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
        var refs2 = config.refs || [];
        for (var ref = 0; ref < refs2.length; ref++) {
          var refValue = Number(refs2[ref].y);
          if (state.log && refValue <= 0) continue;
          var refY = sy(refValue);
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
          var refSide = chooseReferenceLabelSide(
            endpointDistance(panel, sy, xMin, refY),
            endpointDistance(panel, sy, xMax, refY)
          );
          var refText = String(refs2[ref].label || "");
          var refTextX =
            refSide === "right" ? marginLeft + plotWidth - 2 : marginLeft + 2;
          var refAnchor = refSide === "right" ? "end" : "start";
          var refTextY = refY - 6;
          var refNode = addText(
            svg,
            refTextX,
            refTextY,
            refText,
            "cu-ref-label",
            refAnchor
          );
          refLabels.push({
            node: refNode,
            box: textBox(
              family,
              12,
              400,
              refText,
              refTextX,
              refTextY,
              refAnchor
            )
          });
        }

        for (var ls = 0; ls < panel.series.length; ls++) {
          var lineSeries = panel.series[ls];
          var segments = splitLineSegments(lineSeries.points);
          var pathData = "";
          for (var seg = 0; seg < segments.length; seg++) {
            for (var segPoint = 0; segPoint < segments[seg].length; segPoint++) {
              var point = segments[seg][segPoint];
              pathData +=
                (segPoint ? " L " : pathData ? " M " : "M ") +
                round(sx(point.x)) +
                " " +
                round(sy(point.cell.value));
            }
          }
          if (pathData) {
            var line = svgElement(
              "path",
              "cu-line " +
                lineSeries.className +
                (lineSeries.dashed ? " cu-dashed" : "")
            );
            line.setAttribute("d", pathData);
            line.setAttribute("fill", "none");
            svg.appendChild(line);
          }

          for (var dotIndex = 0; dotIndex < lineSeries.points.length; dotIndex++) {
            var dotPoint = lineSeries.points[dotIndex];
            if (isMissingCell(dotPoint.cell)) continue;
            var dot = svgElement("circle", "cu-dot " + lineSeries.className);
            dot.setAttribute("cx", String(round(sx(dotPoint.x))));
            dot.setAttribute("cy", String(round(sy(dotPoint.cell.value))));
            dot.setAttribute("r", "4");
            svg.appendChild(dot);
          }
        }

        // 参考线的文字挪到折线之上，靠描边和折线隔开（窄屏时折线会穿过文字）。
        for (var refLabel = 0; refLabel < refLabels.length; refLabel++) {
          if (refLabels[refLabel]) svg.appendChild(refLabels[refLabel].node);
        }

        if (!multiSeries) {
          var single = panel.series[0];
          var singlePoints = [];
          for (var sp = 0; sp < single.points.length; sp++) {
            if (!isMissingCell(single.points[sp].cell)) {
              singlePoints.push(single.points[sp]);
            }
          }
          if (singlePoints.length) {
            var firstPoint = singlePoints[0];
            var lastPoint = singlePoints[singlePoints.length - 1];
            var firstNeighbor =
              singlePoints.length > 1 ? singlePoints[1] : firstPoint;
            var lastNeighbor =
              singlePoints.length > 1
                ? singlePoints[singlePoints.length - 2]
                : lastPoint;
            var firstOffset = pointLabelOffset(
              "first",
              sy(firstPoint.cell.value),
              sy(firstNeighbor.cell.value)
            );
            var lastOffset = pointLabelOffset(
              "last",
              sy(lastPoint.cell.value),
              sy(lastNeighbor.cell.value)
            );
            addDirectLabel(
              svg,
              refLabels,
              firstPoint.cell.text + (panel.unit ? " " + panel.unit : ""),
              sx(firstPoint.x) + firstOffset.dx,
              sy(firstPoint.cell.value) + firstOffset.dy,
              firstOffset.anchor,
              "cu-point-label",
              400
            );
            addDirectLabel(
              svg,
              refLabels,
              lastPoint.cell.text + (panel.unit ? " " + panel.unit : ""),
              sx(lastPoint.x) + lastOffset.dx,
              sy(lastPoint.cell.value) + lastOffset.dy,
              lastOffset.anchor,
              "cu-point-label",
              400
            );
          }
        }

        panelMeta.push({
          svg: svg,
          sy: sy,
          panel: panel,
          refLabels: refLabels
        });
        svgList.push(svg);
        plot.appendChild(svg);
      }

      addXLabel(
        fig,
        plot,
        config.xLabel || firstPanel.labelName || ""
      );

      // 多条线时只在末端标系列名；标签压到别的线或别的标签时上下挪一个字高，
      // 还挪不开就不画，交给图例和 tooltip。
      if (multiSeries) {
        for (var lp = 0; lp < panelMeta.length; lp++) {
          var meta = panelMeta[lp];
          var lineObstacles = [];
          for (var lsg = 0; lsg < meta.panel.series.length; lsg++) {
            var segSeries = meta.panel.series[lsg];
            var segs = splitLineSegments(segSeries.points);
            for (var sg = 0; sg < segs.length; sg++) {
              for (var sgp = 0; sgp + 1 < segs[sg].length; sgp++) {
                var segA = segs[sg][sgp];
                var segB = segs[sg][sgp + 1];
                lineObstacles.push({
                  owner: lsg,
                  segment: [
                    sx(segA.x),
                    meta.sy(segA.cell.value),
                    sx(segB.x),
                    meta.sy(segB.cell.value)
                  ]
                });
              }
            }
          }
          var placed = [];
          var candidates = [];
          for (var ms = 0; ms < meta.panel.series.length; ms++) {
            var series = meta.panel.series[ms];
            var seriesPoints = [];
            for (var spx = 0; spx < series.points.length; spx++) {
              if (!isMissingCell(series.points[spx].cell)) {
                seriesPoints.push(series.points[spx]);
              }
            }
            if (!seriesPoints.length) continue;
            var last = seriesPoints[seriesPoints.length - 1];
            var previous =
              seriesPoints.length > 1
                ? seriesPoints[seriesPoints.length - 2]
                : last;
            var offset = pointLabelOffset(
              "last",
              meta.sy(last.cell.value),
              meta.sy(previous.cell.value)
            );
            var labelX = sx(last.x) + offset.dx;
            var labelY = meta.sy(last.cell.value) + offset.dy;
            var box = textBox(
              family,
              12,
              600,
              series.name,
              labelX,
              labelY,
              "end"
            );
            candidates.push({
              owner: ms,
              name: series.name,
              x: labelX,
              y: labelY,
              box: box
            });
          }
          candidates.sort(function (a, b) {
            return a.box.top - b.box.top;
          });

          for (var cs = 0; cs < candidates.length; cs++) {
            var candidate = candidates[cs];
            var obstacles = placed.slice();
            for (var lo = 0; lo < lineObstacles.length; lo++) {
              if (lineObstacles[lo].owner !== candidate.owner) {
                obstacles.push(lineObstacles[lo]);
              }
            }
            for (var rb = 0; rb < meta.refLabels.length; rb++) {
              obstacles.push({ box: meta.refLabels[rb].box });
            }
            var shift = findLabelOffset(
              candidate.box,
              obstacles,
              12,
              { top: 2, bottom: marginTop + plotHeight + marginBottom - 2 }
            );
            if (shift === null) continue;
            addText(
              meta.svg,
              candidate.x,
              candidate.y + shift,
              candidate.name,
              "cu-series-label",
              "end"
            );
            placed.push({
              box: {
                left: candidate.box.left,
                right: candidate.box.right,
                top: candidate.box.top + shift,
                bottom: candidate.box.bottom + shift
              }
            });
          }
        }
      }

      function pointForSeries(series, xIndex) {
        var target = xs[xIndex];
        var best = null;
        for (var i = 0; i < series.points.length; i++) {
          if (!best) best = series.points[i];
          else if (
            Math.abs(series.points[i].x - target) <
            Math.abs(best.x - target)
          ) {
            best = series.points[i];
          }
        }
        return best;
      }

      function addActive(index) {
        clearActive();
        var xValue = xs[index];
        var xPx = sx(xValue);
        for (var ai = 0; ai < panelMeta.length; ai++) {
          var panelInfo = panelMeta[ai];
          var crosshair = svgElement("line", "cu-crosshair");
          crosshair.setAttribute("x1", String(round(xPx)));
          crosshair.setAttribute("x2", String(round(xPx)));
          crosshair.setAttribute("y1", String(marginTop));
          crosshair.setAttribute("y2", String(marginTop + plotHeight));
          panelInfo.svg.appendChild(crosshair);
          activeElements.push(crosshair);

          for (var as = 0; as < panelInfo.panel.series.length; as++) {
            var activeSeries = panelInfo.panel.series[as];
            var activePoint = pointForSeries(activeSeries, index);
            if (!activePoint || isMissingCell(activePoint.cell)) continue;
            var activeDot = svgElement(
              "circle",
              "cu-dot cu-active " + activeSeries.className
            );
            activeDot.setAttribute(
              "cx",
              String(round(sx(activePoint.x)))
            );
            activeDot.setAttribute(
              "cy",
              String(round(panelInfo.sy(activePoint.cell.value)))
            );
            activeDot.setAttribute("r", "6");
            panelInfo.svg.appendChild(activeDot);
            activeElements.push(activeDot);
          }
        }
      }

      function tooltipLines(index) {
        if (panels.length === 1 && !multiSeries) {
          // 单条线保持原来的 tooltip：x 一行、数值一行。
          var singleSeries = panels[0].series[0];
          var singlePoint = pointForSeries(singleSeries, index);
          var singleMissing =
            !singlePoint || isMissingCell(singlePoint.cell);
          var singleValue = singleMissing
            ? MISSING_TEXT
            : singlePoint.cell.text +
              (firstPanel.unit ? " " + firstPanel.unit : "");
          var singleLines = [
            {
              keyClass: singleSeries.className,
              text: xTooltipText(index)
            },
            { className: "cu-tip-value", text: singleValue }
          ];
          if (!singleMissing && config.share && config.share.of) {
            var singleRatio =
              (singlePoint.cell.value / Number(config.share.of)) * 100;
            var singlePercent =
              singleRatio < 10
                ? singleRatio.toFixed(1)
                : String(Math.round(singleRatio));
            singleLines.push({
              className: "cu-tip-meta",
              text:
                (config.share.label || "占比") +
                "的 " +
                singlePercent +
                "%"
            });
          }
          if (!singleMissing && singlePoint.note) {
            singleLines.push({
              className: "cu-tip-note",
              text: singlePoint.note
            });
          }
          return singleLines;
        }

        var lines = [
          {
            keyClass: "cu-axis",
            text: xTooltipText(index)
          }
        ];
        var seenNotes = {};
        for (var tp = 0; tp < panels.length; tp++) {
          for (var ts = 0; ts < panels[tp].series.length; ts++) {
            var series = panels[tp].series[ts];
            var point = pointForSeries(series, index);
            if (!point) continue;
            var pointMissing = isMissingCell(point.cell);
            var valueText = pointMissing
              ? MISSING_TEXT
              : pointValueText(point, panels[tp].unit);
            lines.push({
              keyClass: series.className,
              name:
                (panels.length > 1 || multiSeries) && !series.auto
                  ? series.name
                  : undefined,
              value: valueText
            });

            if (!pointMissing && config.share && config.share.of) {
              var ratio = (point.cell.value / Number(config.share.of)) * 100;
              var percent =
                ratio < 10 ? ratio.toFixed(1) : String(Math.round(ratio));
              lines.push({
                className: "cu-tip-meta",
                text:
                  ((panels.length > 1 || multiSeries) && !series.auto
                    ? series.name + " "
                    : "") +
                  (config.share.label || "占比") +
                  "的 " +
                  percent +
                  "%"
              });
            }
            if (!pointMissing && point.note && !seenNotes[point.note]) {
              seenNotes[point.note] = true;
              lines.push({
                className: "cu-tip-note",
                text: point.note
              });
            }
          }
        }
        return lines;
      }

      function showIndex(index, x, y) {
        state.selectedIndex = index;
        state.active = true;
        addActive(index);
        showTip(state, tooltipLines(index), x, y);
      }

      function showIndexAtPoint(index, svg) {
        var xValue = xs[index];
        var anchor = svg || panelMeta[0].svg;
        var anchorInfo = panelMeta[0];
        for (var am = 0; am < panelMeta.length; am++) {
          if (panelMeta[am].svg === anchor) anchorInfo = panelMeta[am];
        }
        var yLocal = marginTop + plotHeight / 2;
        if (anchorInfo.panel.series.length === 1) {
          var focusPoint = pointForSeries(anchorInfo.panel.series[0], index);
          if (focusPoint && !isMissingCell(focusPoint.cell)) {
            yLocal = anchorInfo.sy(focusPoint.cell.value);
          }
        }
        var anchorRect = anchor.getBoundingClientRect();
        var figureRect = fig.getBoundingClientRect();
        showIndex(
          index,
          anchorRect.left - figureRect.left + sx(xValue),
          anchorRect.top - figureRect.top + yLocal
        );
      }

      function hideButKeepSelection() {
        state.active = false;
        hideTip(state);
        clearActive();
      }

      function nearestFromPointer(svg, event) {
        // 换算到整张 SVG 的坐标：overlay 本身从 marginLeft 开始，不能用它的左边当原点。
        var rect = svg.getBoundingClientRect();
        var scale = rect.width ? svgWidth / rect.width : 1;
        var px = (event.clientX - rect.left) * scale;
        if (px < marginLeft) px = marginLeft;
        if (px > marginLeft + plotWidth) px = marginLeft + plotWidth;
        var ratio = (px - plotLeftInner) / (plotRightInner - plotLeftInner);
        if (ratio < 0) ratio = 0;
        if (ratio > 1) ratio = 1;
        var value;
        if (xMax === xMin) {
          value = xMin;
        } else if (xScale === "linear") {
          value = xMin + ratio * (xMax - xMin);
        } else {
          value = Math.exp(
            Math.log(xMin) + ratio * (Math.log(xMax) - Math.log(xMin))
          );
        }
        return nearestIndex(xs, value, xScale !== "linear");
      }

      for (var oi = 0; oi < panelMeta.length; oi++) {
        (function (svg) {
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

          overlay.addEventListener("pointermove", function (event) {
            var index = nearestFromPointer(svg, event);
            var point = figurePoint(fig, event.clientX, event.clientY);
            showIndex(index, point.x, point.y);
          });

          overlay.addEventListener("pointerdown", function (event) {
            var index = nearestFromPointer(svg, event);
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
            showIndexAtPoint(index, svg);
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
              if (index === null) {
                index = event.key === "ArrowRight" ? 0 : xs.length - 1;
              } else {
                index += event.key === "ArrowRight" ? 1 : -1;
              }
              if (index < 0) index = 0;
              if (index > xs.length - 1) index = xs.length - 1;
              showIndexAtPoint(index, svg);
            } else if (event.key === "Home" || event.key === "End") {
              event.preventDefault();
              event.stopPropagation();
              showIndexAtPoint(
                event.key === "Home" ? 0 : xs.length - 1,
                svg
              );
            } else if (event.key === "Escape") {
              hideButKeepSelection();
            }
          });
        })(panelMeta[oi].svg);
      }

      if (state.active && state.selectedIndex !== null) {
        addActive(state.selectedIndex);
        if (restoreTip) {
          showIndexAtPoint(state.selectedIndex);
        }
      }
    };

    state.render();
  }

  /* ------------------------------ roofline ------------------------------ */

  function initRoofline(state) {
    var fig = state.fig;
    var plot = state.plot;
    var config = state.config;

    var unit = config.peakUnit || "TFLOPS";
    var toTflops = function (value) {
      return unit === "GFLOPS" ? Number(value) / 1000 : Number(value);
    };
    var peak = toTflops(config.peak);
    var bandwidth = Number(config.bandwidth);
    var balance = rooflineBalance(peak, bandwidth);
    var points = config.points.map(function (point, index) {
      return {
        label: point.label,
        x: Number(point.x),
        y: toTflops(point.y),
        note: point.note || "",
        index: index
      };
    });

    var left = element("div", "cu-chart-head-meta");
    var yLabel = config.yLabel || "性能（" + unit + "，对数刻度）";
    left.appendChild(element("span", "cu-chart-ylabel", yLabel));
    makeHead(fig, plot, left, makeRightControls());

    var hot = null;

    function clearHot() {
      if (hot) {
        if (hot.circle) hot.circle.classList.remove("cu-hot");
        if (hot.hit) hot.hit.classList.remove("cu-hot");
      }
      hot = null;
    }

    state.hide = function () {
      clearHot();
      hideTip(state);
    };

    state.render = function (requestedWidth) {
      var width = requestedWidth || measureWidth(plot, fig);
      state.width = width;
      clearHot();
      hideTip(state);
      while (plot.firstChild) plot.removeChild(plot.firstChild);

      var family = getComputedStyle(fig).fontFamily || "sans-serif";
      var marginLeft = 62;
      var marginRight = 22;
      var marginTop = 20;
      var marginBottom = 44;
      var plotHeight = 300;
      var plotWidth = Math.max(40, width - marginLeft - marginRight);
      var svgWidth = marginLeft + plotWidth + marginRight;
      var svgHeight = marginTop + plotHeight + marginBottom;

      var pointXs = points.map(function (point) {
        return point.x;
      });
      var pointYs = points.map(function (point) {
        return point.y;
      });
      var xLo =
        config.xMin !== undefined
          ? Number(config.xMin)
          : Math.pow(
              10,
              Math.floor(Math.log10(Math.min.apply(null, pointXs.concat([balance]))))
            );
      var xHi =
        config.xMax !== undefined
          ? Number(config.xMax)
          : Math.pow(
              10,
              Math.ceil(Math.log10(Math.max.apply(null, pointXs.concat([balance]))))
            );
      if (!(xHi > xLo)) xHi = xLo * 10;
      var yLo =
        config.yMin !== undefined
          ? Number(config.yMin)
          : Math.pow(10, Math.floor(Math.log10(Math.min.apply(null, pointYs))));
      var yHi =
        config.yMax !== undefined
          ? Number(config.yMax)
          : Math.pow(
              10,
              Math.ceil(Math.log10(Math.max.apply(null, pointYs.concat([peak]))))
            );
      if (!(yHi > yLo)) yHi = yLo * 10;

      var xTicks = log10Ticks(xLo, xHi);
      if (!xTicks.length) xTicks = [xLo, xHi];
      var yTicks = log10Ticks(yLo, yHi);
      if (!yTicks.length) yTicks = [yLo, yHi];

      function sx(value) {
        return (
          marginLeft +
          ((Math.log10(value) - Math.log10(xLo)) /
            (Math.log10(xHi) - Math.log10(xLo))) *
            plotWidth
        );
      }

      function sy(value) {
        return (
          marginTop +
          plotHeight -
          ((Math.log10(value) - Math.log10(yLo)) /
            (Math.log10(yHi) - Math.log10(yLo))) *
            plotHeight
        );
      }

      var svg = svgElement("svg");
      svg.setAttribute("width", String(svgWidth));
      svg.setAttribute("height", String(svgHeight));
      svg.setAttribute("viewBox", "0 0 " + svgWidth + " " + svgHeight);
      svg.setAttribute("role", "group");
      svg.setAttribute("aria-label", captionText(fig));

      for (var xt = 0; xt < xTicks.length; xt++) {
        var gridX = sx(xTicks[xt]);
        addLine(svg, gridX, marginTop, gridX, marginTop + plotHeight, "cu-grid");
        addText(
          svg,
          gridX,
          marginTop + plotHeight + 17,
          formatTick(xTicks[xt]),
          "cu-tick",
          "middle"
        );
      }
      for (var yt = 0; yt < yTicks.length; yt++) {
        var gridY = sy(yTicks[yt]);
        addLine(svg, marginLeft, gridY, marginLeft + plotWidth, gridY, "cu-grid");
        addText(
          svg,
          marginLeft - 8,
          gridY + 4,
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

      var upper = [];
      if (balance > xLo && balance < xHi) {
        upper.push([xLo, rooflineLimit(xLo, peak, bandwidth)]);
        upper.push([balance, peak]);
        upper.push([xHi, peak]);
      } else if (balance <= xLo) {
        upper.push([xLo, peak]);
        upper.push([xHi, peak]);
      } else {
        upper.push([xLo, rooflineLimit(xLo, peak, bandwidth)]);
        upper.push([xHi, rooflineLimit(xHi, peak, bandwidth)]);
      }
      var upperPath = "";
      for (var up = 0; up < upper.length; up++) {
        upperPath +=
          (up ? " L " : "M ") +
          round(sx(upper[up][0])) +
          " " +
          round(sy(upper[up][1]));
      }
      var upperLine = svgElement("path", "cu-roofline-upper");
      upperLine.setAttribute("d", upperPath);
      upperLine.setAttribute("fill", "none");
      svg.appendChild(upperLine);

      var balanceText = "平衡点 " + Math.round(balance);
      var balanceWidth = textWidth(family, 12, 400, balanceText);
      var balanceX = sx(balance);
      var balanceY = sy(peak);
      var balanceAnchor = "start";
      var balanceLabelX = balanceX + 8;
      if (balanceLabelX + balanceWidth > svgWidth - 4) {
        balanceAnchor = "end";
        balanceLabelX = balanceX - 8;
      }
      addText(
        svg,
        balanceLabelX,
        balanceY - 8,
        balanceText,
        "cu-roofline-balance",
        balanceAnchor
      );
      var balanceBox = textBox(
        family,
        12,
        400,
        balanceText,
        balanceLabelX,
        balanceY - 8,
        balanceAnchor
      );

      var pointBoxes = [];
      var circles = [];
      for (var p = 0; p < points.length; p++) {
        var point = points[p];
        var px = sx(point.x);
        var py = sy(point.y);
        var circle = svgElement("circle", "cu-roofline-point cu-s1");
        circle.setAttribute("cx", String(round(px)));
        circle.setAttribute("cy", String(round(py)));
        circle.setAttribute("r", "5");
        svg.appendChild(circle);
        circles.push(circle);
        pointBoxes.push({
          left: px - 7,
          right: px + 7,
          top: py - 7,
          bottom: py + 7
        });
      }

      var labelObstacles = [{ box: balanceBox }];
      var upperSegments = [];
      for (var us = 0; us + 1 < upper.length; us++) {
        upperSegments.push({
          segment: [
            sx(upper[us][0]),
            sy(upper[us][1]),
            sx(upper[us + 1][0]),
            sy(upper[us + 1][1])
          ]
        });
      }
      var labelCandidates = [
        [9, 4, "start"],
        [-9, 4, "end"],
        [0, -11, "middle"],
        [0, 17, "middle"],
        [9, -8, "start"],
        [-9, -8, "end"]
      ];
      for (var pl = 0; pl < points.length; pl++) {
        var labelPoint = points[pl];
        var labelPx = sx(labelPoint.x);
        var labelPy = sy(labelPoint.y);
        var placed = null;
        for (var lc = 0; lc < labelCandidates.length; lc++) {
          var candidate = labelCandidates[lc];
          var labelX = labelPx + candidate[0];
          var labelY = labelPy + candidate[1];
          var labelBox = textBox(
            family,
            12,
            600,
            labelPoint.label,
            labelX,
            labelY,
            candidate[2]
          );
          if (
            labelBox.left < 2 ||
            labelBox.right > svgWidth - 2 ||
            labelBox.top < 2 ||
            labelBox.bottom > svgHeight - 2
          ) {
            continue;
          }
          var obstacles = labelObstacles
            .concat(
              pointBoxes
                .filter(function (_, boxIndex) {
                  return boxIndex !== pl;
                })
                .map(function (box) {
                  return { box: box };
                })
            )
            .concat(upperSegments);
          if (boxBlocked(labelBox, obstacles)) continue;
          placed = { x: labelX, y: labelY, anchor: candidate[2], box: labelBox };
          break;
        }
        if (!placed) continue;
        addText(
          svg,
          placed.x,
          placed.y,
          labelPoint.label,
          "cu-roofline-label",
          placed.anchor
        );
        labelObstacles.push({ box: placed.box });
      }

      var hits = [];
      for (var ph = 0; ph < points.length; ph++) {
        addPointHit(points[ph], circles[ph], sx(points[ph].x), sy(points[ph].y));
      }

      function addPointHit(point, circle, pointX, pointY) {
        var hit = svgElement("circle", "cu-hit");
        hit.setAttribute("cx", String(round(pointX)));
        hit.setAttribute("cy", String(round(pointY)));
        hit.setAttribute("r", "14");
        hit.setAttribute("tabindex", "0");
        hit.setAttribute(
          "aria-label",
          point.label +
            "，算术强度 " +
            formatTick(point.x) +
            "，性能 " +
            formatTick(point.y) +
            " " +
            unit +
            (point.note ? "，" + point.note : "")
        );

        var limit = rooflineLimit(point.x, peak, bandwidth);
        var percent = limit > 0 ? (point.y / limit) * 100 : 0;
        var percentText =
          Math.abs(percent - Math.round(percent)) < 0.05
            ? String(Math.round(percent))
            : percent.toFixed(1);
        var lines = [
          { keyClass: "cu-s1", text: point.label },
          {
            className: "cu-tip-value",
            text: formatTick(point.y) + " " + unit
          },
          {
            className: "cu-tip-meta",
            text: "算术强度 " + formatTick(point.x) + " 次/字节"
          },
          { className: "cu-tip-meta", text: "占上限 " + percentText + "%" }
        ];
        if (point.note) {
          lines.push({ className: "cu-tip-note", text: point.note });
        }

        var hitIndex = hits.length;
        hits.push(hit);

        function showPoint(x, y) {
          clearHot();
          hot = { circle: circle, hit: hit };
          circle.classList.add("cu-hot");
          hit.classList.add("cu-hot");
          showTip(state, lines, x, y);
        }

        function showAtPoint() {
          var at = plotPoint(fig, plot, pointX, pointY);
          showPoint(at.x, at.y);
        }

        hit.addEventListener("pointerenter", function (event) {
          if (event.pointerType === "touch") return;
          var at = figurePoint(fig, event.clientX, event.clientY);
          showPoint(at.x, at.y);
        });
        hit.addEventListener("pointermove", function (event) {
          var at = figurePoint(fig, event.clientX, event.clientY);
          showPoint(at.x, at.y);
        });
        hit.addEventListener("pointerleave", function () {
          if (document.activeElement !== hit) {
            clearHot();
            hideTip(state);
          }
        });
        hit.addEventListener("pointerdown", function (event) {
          var at = figurePoint(fig, event.clientX, event.clientY);
          showPoint(at.x, at.y);
        });
        hit.addEventListener("focus", showAtPoint);
        hit.addEventListener("blur", function () {
          clearHot();
          hideTip(state);
        });
        hit.addEventListener("keydown", function (event) {
          var key = event.key;
          if (
            key === "ArrowDown" ||
            key === "ArrowRight" ||
            key === "ArrowUp" ||
            key === "ArrowLeft" ||
            key === "Home" ||
            key === "End"
          ) {
            event.preventDefault();
            event.stopPropagation();
            var next = hitIndex;
            if (key === "Home") next = 0;
            else if (key === "End") next = hits.length - 1;
            else if (key === "ArrowDown" || key === "ArrowRight") {
              next = Math.min(hits.length - 1, hitIndex + 1);
            } else {
              next = Math.max(0, hitIndex - 1);
            }
            if (hits[next] && hits[next] !== hit) hits[next].focus();
          } else if (key === "Escape") {
            clearHot();
            hideTip(state);
          }
        });

        svg.appendChild(hit);
      }

      plot.appendChild(svg);
      addXLabel(
        fig,
        plot,
        config.xLabel || "算术强度（对数刻度）"
      );
    };

    state.render();
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", boot);
  } else {
    boot();
  }
})();
