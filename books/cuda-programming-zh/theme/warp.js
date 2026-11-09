/* 第 6 章的两张示意图：block / warp / lane（图 6.1）和 warp 调度（图 6.5）。
 *
 * 这个文件同时是一个普通浏览器脚本和一个 CommonJS 模块：
 *   - mdBook 通过 additional-js 直接加载，初始化页面里的 figure；
 *   - tmp-test-h10/cuda-warp.test.js 用 require() 读取里面的纯函数。
 * 因此文件顶部不碰 document / window，DOM 初始化放在最后的条件分支里。
 */
(function () {
  'use strict';

  var LANE_COUNT = 32;
  var SERIES_COUNT = 4;

  // ---------------------------------------------------------------------------
  // 纯函数：图 6.1 的线程编号，以及图 6.5 的调度模型
  // ---------------------------------------------------------------------------

  function toInt(value, fallback) {
    var n = Number(value);
    return isFinite(n) ? Math.floor(n) : fallback;
  }

  // threadIdx.x -> { warp, lane }。只做一维 block 的换算。
  function threadPartition(threadIdx, threads) {
    var t = Math.max(0, toInt(threadIdx, 0));
    if (threads != null) {
      var total = Math.max(LANE_COUNT, toInt(threads, LANE_COUNT));
      t = Math.min(t, total - 1);
    }
    return { threadIdx: t, warp: Math.floor(t / LANE_COUNT), lane: t % LANE_COUNT };
  }

  function threadToWarp(threadIdx, threads) {
    return threadPartition(threadIdx, threads).warp;
  }

  function threadToLane(threadIdx, threads) {
    return threadPartition(threadIdx, threads).lane;
  }

  // 放大行在窄屏排成两行，每行 16 个；宽屏一行 32 个。
  function laneRowsForWidth(width) {
    var lanes = [];
    for (var i = 0; i < LANE_COUNT; i++) lanes.push(i);
    if (Number(width) < 560) return [lanes.slice(0, 16), lanes.slice(16)];
    return [lanes];
  }

  function formatThreadReadout(threadIdx, threads) {
    var p = threadPartition(threadIdx, threads);
    return 'threadIdx.x = ' + p.threadIdx +
      ' → warp ' + p.warp + '（' + p.threadIdx + ' / 32）' +
      '，lane ' + p.lane + '（' + p.threadIdx + ' % 32）';
  }

  function normalizeProgram(program) {
    var list = Array.isArray(program) ? program : String(program == null ? '' : program).split(',');
    var out = [];
    for (var i = 0; i < list.length; i++) {
      var item = String(list[i]).trim().toUpperCase();
      if (item === 'C' || item === 'L') out.push(item);
    }
    return out.length ? out : ['C'];
  }

  /* 模拟一个调度器在 cycles 个周期里怎样发射指令。
   *
   * 规则：上一个周期发射过的 warp 如果现在能发，就继续发它；否则在能发的
   * warp 里挑最久没发射过的（从没发射过的最久，并列取编号小的）。
   * C 发射后下一周期就能再发；L 发射后要等 latency 个周期。
   *
   * 返回：
   *   launches[c]     第 c 周期发射的 warp 编号，空闲为 -1
   *   instructions[c] 第 c 周期的指令（'C' / 'L'），空闲为 null
   *   states[w][c]    第 c 周期结束后 warp w 的状态：
   *                   { state: 'issue'|'ready'|'wait', remaining, ... }
   */
  function simulate(program, latency, warps, cycles) {
    var prog = normalizeProgram(program);
    var lat = Math.max(1, toInt(latency, 1));
    var warpCount = Math.max(1, toInt(warps, 1));
    var total = Math.max(0, toInt(cycles, 0));

    var pc = [];
    var issueCount = [];
    var lastIssue = [];
    var nextEligible = [];
    var launches = [];
    var instructions = [];
    var states = [];
    var w;
    for (w = 0; w < warpCount; w++) {
      pc[w] = 0;
      issueCount[w] = 0;
      lastIssue[w] = -Infinity;
      nextEligible[w] = 0;
      states[w] = [];
    }

    var lastWarp = -1;
    for (var c = 0; c < total; c++) {
      function canIssue(warp) {
        return nextEligible[warp] <= c;
      }

      var chosen = -1;
      if (lastWarp >= 0 && canIssue(lastWarp)) {
        chosen = lastWarp;
      } else {
        var best = -1;
        var bestLast = Infinity;
        for (w = 0; w < warpCount; w++) {
          if (!canIssue(w)) continue;
          if (lastIssue[w] < bestLast) {
            best = w;
            bestLast = lastIssue[w];
          }
        }
        chosen = best;
      }

      var instruction = null;
      if (chosen >= 0) {
        instruction = prog[pc[chosen] % prog.length];
        launches.push(chosen);
        instructions.push(instruction);
        lastIssue[chosen] = c;
        issueCount[chosen] += 1;
        pc[chosen] = (pc[chosen] + 1) % prog.length;
        nextEligible[chosen] = c + (instruction === 'L' ? lat : 1);
      } else {
        launches.push(-1);
        instructions.push(null);
      }

      for (w = 0; w < warpCount; w++) {
        var state;
        var remaining = 0;
        if (chosen === w) {
          state = 'issue';
        } else if (nextEligible[w] <= c) {
          state = 'ready';
        } else {
          state = 'wait';
          remaining = Math.max(0, nextEligible[w] - c);
        }
        states[w].push({
          state: state,
          remaining: remaining,
          nextEligible: nextEligible[w],
          issueCount: issueCount[w],
          nextPc: pc[w],
          instruction: chosen === w ? instruction : null
        });
      }
      lastWarp = chosen;
    }

    var idle = 0;
    for (var i = 0; i < launches.length; i++) if (launches[i] < 0) idle += 1;
    var issued = total - idle;

    return {
      program: prog,
      latency: lat,
      warps: warpCount,
      cycles: total,
      launches: launches,
      instructions: instructions,
      states: states,
      issuedCounts: issueCount.slice(),
      issuedCycles: issued,
      idleCycles: idle,
      utilization: total ? Math.round((issued / total) * 100) : 0
    };
  }

  // ---------------------------------------------------------------------------
  // 小块 DOM 工具
  // ---------------------------------------------------------------------------

  function el(tag, className, text) {
    var node = document.createElement(tag);
    if (className) node.className = className;
    if (text != null) node.textContent = text;
    return node;
  }

  function svgEl(tag, attrs) {
    var node = document.createElementNS('http://www.w3.org/2000/svg', tag);
    if (attrs) {
      for (var key in attrs) {
        if (Object.prototype.hasOwnProperty.call(attrs, key)) {
          node.setAttribute(key, attrs[key]);
        }
      }
    }
    return node;
  }

  function seriesVar(index) {
    return 'var(--cu-s' + ((index % SERIES_COUNT) + 1) + ')';
  }

  function colorVar(index) {
    return 'var(--cu-w' + (index % 8) + ')';
  }

  function parseWarpList(value, fallback) {
    var list = String(value == null ? '' : value).split(',');
    var out = [];
    for (var i = 0; i < list.length; i++) {
      var n = toInt(list[i], -1);
      if (n > 0 && out.indexOf(n) < 0) out.push(n);
    }
    return out.length ? out : fallback;
  }

  function ifigure(figure) {
    return {
      threads: Math.max(LANE_COUNT, Math.min(256, toInt(figure.dataset.threads, 128))),
      instruction: figure.dataset.instruction || 'FFMA R7, R2, UR6, R7'
    };
  }

  function normalizeThreads(value) {
    var n = Math.max(LANE_COUNT, Math.min(256, toInt(value, 128)));
    return Math.floor(n / LANE_COUNT) * LANE_COUNT || LANE_COUNT;
  }

  function announceRegion(className) {
    var node = el('div', 'cu-sr-only ' + (className || ''));
    node.setAttribute('aria-live', 'polite');
    node.setAttribute('aria-atomic', 'true');
    return node;
  }

  // ---------------------------------------------------------------------------
  // 图 6.1：block / warp / lane
  // ---------------------------------------------------------------------------

  function initWarpFigure(figure) {
    if (!figure || figure.dataset.cuWarpReady === 'true') return;
    figure.dataset.cuWarpReady = 'true';

    var data = ifigure(figure);
    var threads = normalizeThreads(data.threads);
    var warpCount = threads / LANE_COUNT;

    var body = el('div', 'cu-warp-body');
    var canvas = el('div', 'cu-warp-canvas');
    var readout = el('div', 'cu-warp-readout');
    var live = announceRegion('cu-warp-live');
    body.appendChild(canvas);
    body.appendChild(readout);
    body.appendChild(live);

    var caption = figure.querySelector('figcaption');
    if (caption) figure.insertBefore(body, caption);
    else figure.appendChild(body);

    var state = {
      width: 0,
      rows: [],
      warp: warpCount > 1 ? 1 : 0,
      selected: null,
      hover: null,
      focus: null,
      cells: [],
      segs: [],
      ticks: [],
      trap: [],
      cellsGroup: null,
      segW: 0
    };

    function displayThread() {
      if (state.hover != null) return state.hover;
      if (state.focus != null) return state.focus;
      return state.selected;
    }

    function announce(message) {
      live.textContent = '';
      window.setTimeout(function () {
        live.textContent = message;
      }, 30);
    }

    function setReadout(threadIdx) {
      readout.textContent = '';
      if (threadIdx == null) {
        readout.classList.add('is-default');
        readout.textContent = '指到一个线程，看它的编号';
        return;
      }
      readout.classList.remove('is-default');
      var p = threadPartition(threadIdx, threads);
      var code = function (text) {
        var c = el('code', null, text);
        readout.appendChild(c);
      };
      code('threadIdx.x');
      readout.appendChild(document.createTextNode(' = '));
      code(String(p.threadIdx));
      readout.appendChild(document.createTextNode(' → warp '));
      code(String(p.warp));
      readout.appendChild(document.createTextNode('（'));
      code(p.threadIdx + ' / 32');
      readout.appendChild(document.createTextNode('），lane '));
      code(String(p.lane));
      readout.appendChild(document.createTextNode('（'));
      code(p.threadIdx + ' % 32');
      readout.appendChild(document.createTextNode('）'));
    }

    function paint() {
      var active = displayThread();
      var warp = state.warp;

      for (var i = 0; i < state.segs.length; i++) {
        var selected = i === warp;
        state.segs[i].classList.toggle('is-selected', selected);
        state.segs[i].setAttribute('aria-pressed', selected ? 'true' : 'false');
        var rangeStart = i * LANE_COUNT;
        state.segs[i].setAttribute(
          'aria-label',
          'warp ' + i + '，threadIdx.x ' + rangeStart + ' 到 ' + (rangeStart + LANE_COUNT - 1)
        );
      }

      for (var t = 0; t < state.ticks.length; t++) {
        state.ticks[t].classList.toggle('is-active', t === active);
      }

      if (state.cellsGroup) {
        state.cellsGroup.style.setProperty('--warp-color', seriesVar(warp));
      }

      for (var lane = 0; lane < LANE_COUNT; lane++) {
        var threadIdx = warp * LANE_COUNT + lane;
        var cell = state.cells[lane];
        if (!cell) continue;
        cell.setAttribute('data-thread', String(threadIdx));
        cell.classList.toggle('is-active', threadIdx === active);
        cell.classList.toggle('is-selected', threadIdx === state.selected);
        cell.setAttribute('aria-pressed', threadIdx === state.selected ? 'true' : 'false');
        cell.setAttribute('aria-label', 'lane ' + lane + '，threadIdx.x ' + threadIdx);
        var laneText = cell.querySelector('.cu-warp-lane-num');
        var threadText = cell.querySelector('.cu-warp-thread-num');
        if (laneText) laneText.textContent = String(lane);
        if (threadText) {
          threadText.textContent = String(threadIdx);
          var textWidth = state.rows.length > 1 ? state.cellW - 1 : state.cellW - 1;
          if (String(threadIdx).length > 2 && textWidth < 23) {
            threadText.setAttribute('textLength', String(Math.max(16, textWidth)));
            threadText.setAttribute('lengthAdjust', 'spacingAndGlyphs');
          } else {
            threadText.removeAttribute('textLength');
            threadText.removeAttribute('lengthAdjust');
          }
        }
      }

      if (state.trap.length === 2 && state.segW) {
        var left = warp * state.segW + 2;
        var right = (warp + 1) * state.segW - 2;
        state.trap[0].setAttribute('x1', left);
        state.trap[1].setAttribute('x1', right);
      }

      setReadout(active);
    }

    function selectWarp(warp, notify) {
      state.warp = warp;
      state.selected = null;
      state.hover = null;
      state.focus = null;
      paint();
      if (notify) announce('已选中 warp ' + warp + '，threadIdx.x ' + (warp * LANE_COUNT) + ' 到 ' + (warp * LANE_COUNT + LANE_COUNT - 1));
    }

    function selectThread(threadIdx, notify) {
      var p = threadPartition(threadIdx, threads);
      state.warp = p.warp;
      state.selected = p.threadIdx;
      state.hover = null;
      state.focus = null;
      paint();
      if (notify) announce(formatThreadReadout(p.threadIdx, threads));
    }

    function hoverThread(threadIdx) {
      var p = threadPartition(threadIdx, threads);
      if (p.warp !== state.warp) {
        state.warp = p.warp;
        state.selected = null;
        state.focus = null;
      }
      state.hover = p.threadIdx;
      paint();
    }

    function focusThread(threadIdx) {
      var p = threadPartition(threadIdx, threads);
      state.warp = p.warp;
      state.focus = p.threadIdx;
      state.hover = null;
      paint();
      announce(formatThreadReadout(p.threadIdx, threads));
    }

    function moveLaneFocus(fromThread, delta) {
      var p = threadPartition(fromThread, threads);
      var nextLane = (p.lane + delta + LANE_COUNT) % LANE_COUNT;
      var next = state.cells[nextLane];
      if (next) next.focus();
    }

    function build() {
      var width = Math.max(280, Math.floor(body.clientWidth || figure.clientWidth || 750));
      var rows = laneRowsForWidth(width);
      var W = width;
      var blockTitleY = 18;
      var blockY = 30;
      var blockH = 52;
      var blockBottom = blockY + blockH;
      var warpLabelY = 102;
      var rangeY = 120;
      var boxY = 148;
      var boxH = 36;
      var labelY = 204;
      var laneY = 246;
      var cellH = 44;
      var rowGap = 10;
      var laneBottom = laneY + rows.length * cellH + (rows.length - 1) * rowGap;
      var H = laneBottom + 8;
      var segW = W / warpCount;
      var gap = 1;
      // 左右各留 1px，格子总宽严格等于 viewBox 宽度，避免页面横向溢出。
      var cellW = (W - gap * (rows[0].length + 1)) / rows[0].length;

      state.width = width;
      state.rows = rows;
      state.segW = segW;
      state.cellW = cellW;

      var svg = svgEl('svg', {
        viewBox: '0 0 ' + W + ' ' + H,
        role: 'group',
        'aria-label': 'block 里的线程分成 warp，每个 warp 有 32 个 lane'
      });
      svg.classList.add('cu-warp-svg');
      svg.style.width = '100%';
      svg.style.height = 'auto';

      // 标题
      var title = svgEl('text', { x: 0, y: blockTitleY, class: 'cu-warp-title' });
      title.appendChild(document.createTextNode('block（' + threads + ' 个线程，'));
      var titleCode = svgEl('tspan', { class: 'cu-warp-mono' });
      titleCode.textContent = 'threadIdx.x 0–' + (threads - 1);
      title.appendChild(titleCode);
      title.appendChild(document.createTextNode('）'));
      svg.appendChild(title);

      // 梯形放大线先画，后面用过背景的框和格子盖住穿过中间的部分。
      var trapG = svgEl('g', { class: 'cu-warp-traps', 'aria-hidden': 'true' });
      state.trap = [];
      for (var ti = 0; ti < 2; ti++) {
        var line = svgEl('line', {
          y1: blockBottom,
          x2: ti === 0 ? 0 : W,
          y2: laneY,
          class: 'cu-warp-trap'
        });
        trapG.appendChild(line);
        state.trap.push(line);
      }
      svg.appendChild(trapG);

      // block 条和 128 个线程刻度
      var blockG = svgEl('g', { class: 'cu-warp-block' });
      state.segs = [];
      state.ticks = [];
      for (var w = 0; w < warpCount; w++) {
        var segX = w * segW;
        var seg = svgEl('g', {
          class: 'cu-warp-seg',
          tabindex: '0',
          role: 'button',
          'aria-pressed': 'false',
          style: '--warp-color: ' + seriesVar(w)
        });
        var bg = svgEl('rect', {
          x: segX + 0.5,
          y: blockY,
          width: Math.max(1, segW - 1),
          height: blockH,
          rx: 4,
          class: 'cu-warp-seg-bg'
        });
        seg.appendChild(bg);

        for (var lane = 0; lane < LANE_COUNT; lane++) {
          var laneX = segX + (lane + 0.5) * (segW / LANE_COUNT);
          var tick = svgEl('line', {
            x1: laneX,
            x2: laneX,
            y1: blockY + 7,
            y2: blockY + blockH - 7,
            class: 'cu-warp-tick'
          });
          seg.appendChild(tick);
          state.ticks.push(tick);
        }

        // 刻度线很细，用透明的命中区扩大可点范围；事件在下面统一绑定。
        var hits = [];
        for (var h = 0; h < LANE_COUNT; h++) {
          var hit = svgEl('rect', {
            x: segX + h * (segW / LANE_COUNT),
            y: blockY,
            width: Math.max(1, segW / LANE_COUNT),
            height: blockH,
            fill: 'transparent',
            class: 'cu-warp-hit'
          });
          seg.appendChild(hit);
          hits.push(hit);
        }
        blockG.appendChild(seg);
        state.segs.push(seg);

        var warpLabel = svgEl('text', {
          x: segX + segW / 2,
          y: warpLabelY,
          class: 'cu-warp-warp-label',
          'text-anchor': 'middle'
        });
        warpLabel.textContent = 'warp ' + w;
        blockG.appendChild(warpLabel);

        var range = svgEl('text', {
          x: segX + segW / 2,
          y: rangeY,
          class: 'cu-warp-range',
          'text-anchor': 'middle'
        });
        range.textContent = (w * LANE_COUNT) + '–' + (w * LANE_COUNT + LANE_COUNT - 1);
        blockG.appendChild(range);

        (function (warpIndex, hitEls) {
          var segEl = state.segs[warpIndex];
          segEl.addEventListener('click', function (event) {
            event.stopPropagation();
            selectWarp(warpIndex, true);
          });
          segEl.addEventListener('keydown', function (event) {
            if (event.key === 'Enter' || event.key === ' ' || event.key === 'Spacebar') {
              event.preventDefault();
              event.stopPropagation();
              selectWarp(warpIndex, true);
            }
          });
          for (var k = 0; k < hitEls.length; k++) {
            (function (hitEl, threadIdx) {
              hitEl.addEventListener('pointerenter', function () {
                hoverThread(threadIdx);
              });
              hitEl.addEventListener('click', function (event) {
                event.stopPropagation();
                selectThread(threadIdx, true);
              });
            })(hitEls[k], warpIndex * LANE_COUNT + k);
          }
        })(w, hits);
      }
      blockG.addEventListener('pointerleave', function () {
        if (state.hover != null) {
          state.hover = null;
          paint();
        }
      });
      svg.appendChild(blockG);

      // 指令框
      var boxW = Math.min(360, W - 40);
      var boxX = (W - boxW) / 2;
      var boxG = svgEl('g', { class: 'cu-warp-instruction' });
      boxG.appendChild(svgEl('rect', {
        x: boxX,
        y: boxY,
        width: boxW,
        height: boxH,
        rx: 6,
        class: 'cu-warp-instr-box'
      }));
      var instrText = svgEl('text', {
        x: W / 2,
        y: boxY + 24,
        class: 'cu-warp-mono cu-warp-instr-text',
        'text-anchor': 'middle'
      });
      instrText.textContent = data.instruction;
      boxG.appendChild(instrText);
      svg.appendChild(boxG);

      // 梳状分发线：指令 -> 总线 -> 每个 lane。窄屏两行时画两条总线。
      var busG = svgEl('g', { class: 'cu-warp-bus', 'aria-hidden': 'true' });
      var bus1Y = laneY - 16;
      busG.appendChild(svgEl('line', {
        x1: W / 2, x2: W / 2, y1: boxY + boxH, y2: bus1Y, class: 'cu-warp-bus-line'
      }));
      busG.appendChild(svgEl('line', {
        x1: 8, x2: W - 8, y1: bus1Y, y2: bus1Y, class: 'cu-warp-bus-line'
      }));
      if (rows.length > 1) {
        var bus2Y = laneY + cellH + rowGap - 16;
        busG.appendChild(svgEl('line', {
          x1: 8, x2: 8, y1: bus1Y, y2: bus2Y, class: 'cu-warp-bus-line'
        }));
        busG.appendChild(svgEl('line', {
          x1: 8, x2: W - 8, y1: bus2Y, y2: bus2Y, class: 'cu-warp-bus-line'
        }));
      }
      for (var r = 0; r < rows.length; r++) {
        var busY = laneY + r * (cellH + rowGap) - 16;
        for (var c = 0; c < rows[r].length; c++) {
          var cx = gap + c * (cellW + gap) + cellW / 2;
          busG.appendChild(svgEl('line', {
            x1: cx, x2: cx, y1: busY, y2: laneY + r * (cellH + rowGap), class: 'cu-warp-drop'
          }));
        }
      }
      svg.appendChild(busG);

      // 说明文字压在分发线上，加一块背景保证可读。
      var labelG = svgEl('g', { class: 'cu-warp-fan-labels' });
      var leftLabel = svgEl('text', {
        x: W / 2 - 8, y: labelY, class: 'cu-warp-fan-label', 'text-anchor': 'end'
      });
      leftLabel.textContent = '一条指令';
      var rightLabel = svgEl('text', {
        x: W / 2 + 8, y: labelY, class: 'cu-warp-fan-label', 'text-anchor': 'start'
      });
      rightLabel.textContent = '32 个 lane 同时执行';
      labelG.appendChild(leftLabel);
      labelG.appendChild(rightLabel);
      svg.appendChild(labelG);

      // 放大行的 32 个格子
      var cellsG = svgEl('g', { class: 'cu-warp-cells', style: '--warp-color: ' + seriesVar(state.warp) });
      state.cells = [];
      state.cellsGroup = cellsG;
      for (var rowIndex = 0; rowIndex < rows.length; rowIndex++) {
        var rowY = laneY + rowIndex * (cellH + rowGap);
        for (var col = 0; col < rows[rowIndex].length; col++) {
          var laneNum = rows[rowIndex][col];
          var cellX = gap + col * (cellW + gap);
          var cell = svgEl('g', {
            class: 'cu-warp-cell',
            tabindex: '0',
            role: 'button',
            'aria-pressed': 'false',
            'data-lane': laneNum
          });
          cell.appendChild(svgEl('rect', {
            x: cellX,
            y: rowY,
            width: cellW,
            height: cellH,
            rx: 4,
            class: 'cu-warp-cell-bg'
          }));
          var laneText = svgEl('text', {
            x: cellX + cellW / 2,
            y: rowY + 21,
            class: 'cu-warp-lane-num',
            'text-anchor': 'middle'
          });
          laneText.textContent = String(laneNum);
          cell.appendChild(laneText);
          var threadText = svgEl('text', {
            x: cellX + cellW / 2,
            y: rowY + 37,
            class: 'cu-warp-thread-num',
            'text-anchor': 'middle'
          });
          threadText.textContent = String(state.warp * LANE_COUNT + laneNum);
          cell.appendChild(threadText);
          cellsG.appendChild(cell);
          state.cells.push(cell);

          (function (cellEl, laneIndex) {
            cellEl.addEventListener('pointerenter', function () {
              hoverThread(state.warp * LANE_COUNT + laneIndex);
            });
            cellEl.addEventListener('pointerleave', function () {
              if (state.hover === state.warp * LANE_COUNT + laneIndex) {
                state.hover = null;
                paint();
              }
            });
            cellEl.addEventListener('focus', function () {
              focusThread(state.warp * LANE_COUNT + laneIndex);
            });
            cellEl.addEventListener('blur', function () {
              if (state.focus === state.warp * LANE_COUNT + laneIndex) {
                state.focus = null;
                paint();
              }
            });
            cellEl.addEventListener('click', function (event) {
              event.stopPropagation();
              selectThread(state.warp * LANE_COUNT + laneIndex, true);
            });
            cellEl.addEventListener('keydown', function (event) {
              if (event.key === 'ArrowLeft' || event.key === 'ArrowRight') {
                event.preventDefault();
                event.stopPropagation();
                moveLaneFocus(state.warp * LANE_COUNT + laneIndex, event.key === 'ArrowLeft' ? -1 : 1);
              } else if (event.key === 'Enter' || event.key === ' ' || event.key === 'Spacebar') {
                event.preventDefault();
                event.stopPropagation();
                selectThread(state.warp * LANE_COUNT + laneIndex, true);
              }
            });
          })(cell, laneNum);
        }
      }
      svg.appendChild(cellsG);

      // 键盘焦点在 SVG 元素上时，浏览器不一定画 outline，给格子一个显式边框。
      svg.addEventListener('pointerleave', function () {
        if (state.hover != null) {
          state.hover = null;
          paint();
        }
      });

      canvas.replaceChildren(svg);
      paint();
    }

    function resizeIfNeeded() {
      var width = Math.floor(body.clientWidth || figure.clientWidth || 0);
      if (!width) return;
      var oldMode = laneRowsForWidth(state.width).length;
      var newMode = laneRowsForWidth(width).length;
      if (!state.width || Math.abs(width - state.width) > 2 || oldMode !== newMode) build();
    }

    build();
    if (typeof ResizeObserver === 'function') {
      var ro = new ResizeObserver(function () { resizeIfNeeded(); });
      ro.observe(body);
    } else {
      window.addEventListener('resize', resizeIfNeeded);
    }

    window.addEventListener('beforeprint', function () {
      // 打印时回到默认 warp，得到一张不依赖鼠标位置的静态图。
      state.warp = warpCount > 1 ? 1 : 0;
      state.selected = null;
      state.hover = null;
      state.focus = null;
      paint();
    });
  }

  // ---------------------------------------------------------------------------
  // 图 6.5：warp 调度
  // ---------------------------------------------------------------------------

  function initWarpschedFigure(figure) {
    if (!figure || figure.dataset.cuWarpschedReady === 'true') return;
    figure.dataset.cuWarpschedReady = 'true';

    var cycles = Math.max(1, toInt(figure.dataset.cycles, 38));
    var latency = Math.max(1, toInt(figure.dataset.latency, 16));
    var program = normalizeProgram(figure.dataset.program || 'C,C,C,L');
    var warpChoices = parseWarpList(figure.dataset.warps, [1, 2, 4, 5, 8]);
    if (warpChoices.indexOf(1) < 0) warpChoices.unshift(1);

    var state = {
      warps: 1,
      elapsed: 0,
      playing: false,
      timer: null,
      lastLaunch: -1,
      lastInstruction: null,
      printSnapshot: null
    };

    var body = el('div', 'cu-warpsched-body');
    var controls = el('div', 'cu-warpsched-controls');
    var picker = el('div', 'cu-warpsched-picker');
    var pickerLabel = el('span', 'cu-warpsched-picker-label', '调度器名下的 warp 数');
    var pickerButtons = el('div', 'cu-warpsched-warpbuttons');
    pickerButtons.setAttribute('role', 'group');
    pickerButtons.setAttribute('aria-label', '调度器名下的 warp 数');
    picker.appendChild(pickerLabel);
    picker.appendChild(pickerButtons);

    var warpButtons = [];
    for (var i = 0; i < warpChoices.length; i++) {
      (function (n) {
        var button = el('button', 'cu-warpsched-warpbtn', String(n));
        button.type = 'button';
        button.setAttribute('aria-pressed', 'false');
        button.dataset.warps = String(n);
        button.addEventListener('click', function () { setWarpCount(n, true); });
        pickerButtons.appendChild(button);
        warpButtons.push(button);
      })(warpChoices[i]);
    }

    var transport = el('div', 'cu-warpsched-transport');
    transport.setAttribute('role', 'group');
    transport.setAttribute('aria-label', '播放控制');
    var playButton = el('button', 'cu-warpsched-button', '播放');
    playButton.type = 'button';
    var stepButton = el('button', 'cu-warpsched-button', '单步');
    stepButton.type = 'button';
    var resetButton = el('button', 'cu-warpsched-button', '重来');
    resetButton.type = 'button';
    transport.appendChild(playButton);
    transport.appendChild(stepButton);
    transport.appendChild(resetButton);

    var cycleReadout = el('div', 'cu-warpsched-cycle');
    var cycleNow = el('span', 'cu-warpsched-cycle-now', '0');
    var cycleTotal = el('span', 'cu-warpsched-cycle-total', String(cycles));
    cycleReadout.appendChild(document.createTextNode('第 '));
    cycleReadout.appendChild(cycleNow);
    cycleReadout.appendChild(document.createTextNode(' / '));
    cycleReadout.appendChild(cycleTotal);
    cycleReadout.appendChild(document.createTextNode(' 周期'));

    controls.appendChild(picker);
    controls.appendChild(transport);
    controls.appendChild(cycleReadout);

    var stage = el('div', 'cu-warpsched-stage');
    var rowsWrap = el('div', 'cu-warpsched-rows');
    rowsWrap.setAttribute('role', 'list');
    rowsWrap.setAttribute('aria-label', 'warp 状态');
    var arrow = el('div', 'cu-warpsched-arrow');
    arrow.setAttribute('aria-hidden', 'true');
    var arrowLine = el('span', 'cu-warpsched-arrow-line');
    var arrowHead = el('span', 'cu-warpsched-arrow-head');
    arrow.appendChild(arrowLine);
    arrow.appendChild(arrowHead);
    var sched = el('div', 'cu-warpsched-sched');
    var schedBox = el('div', 'cu-warpsched-sched-box');
    var schedName = el('span', 'cu-warpsched-sched-name', '调度器');
    var schedState = el('span', 'cu-warpsched-sched-state', '待命');
    schedBox.appendChild(schedName);
    schedBox.appendChild(schedState);
    sched.appendChild(schedBox);
    stage.appendChild(rowsWrap);
    stage.appendChild(arrow);
    stage.appendChild(sched);

    var timeline = el('div', 'cu-warpsched-timeline');
    timeline.setAttribute('role', 'group');
    timeline.setAttribute('aria-label', '发射时间线');
    var timelineCells = [];
    var half = Math.ceil(cycles / 2);
    for (var rowIndex = 0; rowIndex < 2; rowIndex++) {
      var start = rowIndex * half;
      var end = Math.min(cycles, start + half);
      if (start >= end) break;
      var line = el('div', 'cu-warpsched-timeline-row');
      var label = el('div', 'cu-warpsched-timeline-label', '周期 ' + start + '–' + (end - 1));
      var cells = el('div', 'cu-warpsched-timeline-cells');
      cells.style.setProperty('--cols', String(end - start));
      for (var c = start; c < end; c++) {
        var cell = el('div', 'cu-warpsched-cell');
        cell.dataset.cycle = String(c);
        cell.setAttribute('role', 'img');
        cells.appendChild(cell);
        timelineCells.push(cell);
      }
      line.appendChild(label);
      line.appendChild(cells);
      timeline.appendChild(line);
    }

    var stats = el('div', 'cu-warpsched-stats');
    var live = announceRegion('cu-warpsched-live');
    var notes = figure.querySelector('.cu-warpsched-notes');
    var caption = figure.querySelector('figcaption');

    body.appendChild(controls);
    body.appendChild(stage);
    body.appendChild(timeline);
    body.appendChild(stats);
    body.appendChild(live);
    if (notes) figure.insertBefore(body, notes);
    else if (caption) figure.insertBefore(body, caption);
    else figure.appendChild(body);

    var rowEls = [];
    var seqEls = [];
    var statusEls = [];
    var waitEls = [];
    var noteEls = notes ? Array.prototype.slice.call(notes.querySelectorAll('.cu-warpsched-note')) : [];

    function announce(message) {
      live.textContent = '';
      window.setTimeout(function () {
        live.textContent = message;
      }, 30);
    }

    function reducedMotion() {
      return typeof window.matchMedia === 'function' &&
        window.matchMedia('(prefers-reduced-motion: reduce)').matches;
    }

    function stopTimer() {
      if (state.timer != null) {
        window.clearTimeout(state.timer);
        state.timer = null;
      }
    }

    function setPlaying(playing, notify) {
      state.playing = playing;
      playButton.textContent = playing ? '暂停' : '播放';
      playButton.setAttribute('aria-label', playing ? '暂停播放' : '播放');
      if (!playing) stopTimer();
      if (notify) announce(playing ? '开始播放' : '已暂停');
    }

    function setWarpCount(n, focusButton) {
      setPlaying(false, false);
      state.warps = n;
      state.elapsed = 0;
      updateWarpButtons();
      buildRows();
      render(false);
      if (focusButton) {
        for (var i = 0; i < warpButtons.length; i++) {
          if (Number(warpButtons[i].dataset.warps) === n) {
            warpButtons[i].focus();
            break;
          }
        }
      }
      announce('改成 ' + n + ' 个 warp，回到第 0 周期');
    }

    function updateWarpButtons() {
      for (var i = 0; i < warpButtons.length; i++) {
        var n = Number(warpButtons[i].dataset.warps);
        warpButtons[i].setAttribute('aria-pressed', n === state.warps ? 'true' : 'false');
        warpButtons[i].classList.toggle('is-current', n === state.warps);
      }
    }

    function buildRows() {
      rowsWrap.textContent = '';
      rowEls = [];
      seqEls = [];
      statusEls = [];
      waitEls = [];
      var seqCount = Math.min(program.length * 2, 8);

      for (var w = 0; w < state.warps; w++) {
        var row = el('div', 'cu-warpsched-row');
        row.dataset.warp = String(w);
        row.style.setProperty('--warp-color', colorVar(w));
        row.setAttribute('role', 'listitem');

        var head = el('div', 'cu-warpsched-row-head');
        var swatch = el('span', 'cu-warpsched-swatch');
        swatch.setAttribute('aria-hidden', 'true');
        var name = el('span', 'cu-warpsched-row-name', 'warp ' + w);
        head.appendChild(swatch);
        head.appendChild(name);

        var seq = el('div', 'cu-warpsched-seq');
        seq.setAttribute('aria-hidden', 'true');
        var items = [];
        for (var s = 0; s < seqCount; s++) {
          var instruction = program[s % program.length];
          var item = el('span', 'cu-warpsched-seq-item' + (instruction === 'L' ? ' is-load' : ''), instruction);
          item.title = instruction === 'L' ? '读显存' : '计算';
          seq.appendChild(item);
          items.push(item);
        }

        var status = el('div', 'cu-warpsched-status');
        var statusText = el('span', 'cu-warpsched-status-text', '就绪');
        var waitBar = el('span', 'cu-warpsched-waitbar');
        waitBar.setAttribute('aria-hidden', 'true');
        var waitFill = el('span', 'cu-warpsched-waitfill');
        waitBar.appendChild(waitFill);
        status.appendChild(statusText);
        status.appendChild(waitBar);

        row.appendChild(head);
        row.appendChild(seq);
        row.appendChild(status);
        rowsWrap.appendChild(row);

        rowEls.push(row);
        seqEls.push(items);
        statusEls.push({ text: statusText, bar: waitBar, fill: waitFill });
        waitEls.push(waitBar);
      }
    }

    function updateTimeline(sim, activeCycle) {
      for (var c = 0; c < timelineCells.length; c++) {
        var cell = timelineCells[c];
        var filled = c < state.elapsed && sim.launches[c] >= 0;
        var warp = filled ? sim.launches[c] : -1;
        var instruction = c < state.elapsed ? sim.instructions[c] : null;
        cell.classList.toggle('is-filled', filled);
        cell.classList.toggle('is-idle', c < state.elapsed && !filled);
        cell.classList.toggle('is-load', instruction === 'L');
        cell.classList.toggle('is-current', c === activeCycle);
        if (filled) {
          cell.textContent = String(warp);
          cell.style.setProperty('--warp-color', colorVar(warp));
          cell.setAttribute(
            'aria-label',
            '周期 ' + c + '：warp ' + warp + (instruction === 'L' ? '，读显存' : '，计算')
          );
        } else if (c < state.elapsed) {
          cell.textContent = '';
          cell.style.removeProperty('--warp-color');
          cell.setAttribute('aria-label', '周期 ' + c + '：空闲');
        } else {
          cell.textContent = '';
          cell.style.removeProperty('--warp-color');
          cell.setAttribute('aria-label', '周期 ' + c + '：未执行');
        }
        if (c === activeCycle) cell.setAttribute('aria-current', 'true');
        else cell.removeAttribute('aria-current');
      }
    }

    function updateRows(sim) {
      for (var w = 0; w < state.warps; w++) {
        var row = rowEls[w];
        var info = state.elapsed > 0
          ? sim.states[w][state.elapsed - 1]
          : { state: 'ready', remaining: 0, issueCount: 0, nextPc: 0 };
        var status = info.state;
        var statusText = status === 'issue' ? '发射中' : (status === 'ready' ? '就绪' : '等数据 ' + info.remaining);
        statusEls[w].text.textContent = statusText;
        row.classList.toggle('is-issued-now', status === 'issue');
        row.classList.toggle('is-wait', status === 'wait');
        row.setAttribute('aria-label', 'warp ' + w + '，' + statusText);

        var seqCount = seqEls[w].length;
        for (var s = 0; s < seqCount; s++) {
          var item = seqEls[w][s];
          var isIssued = s < info.issueCount;
          var isNext = s === info.issueCount && info.issueCount < seqCount;
          item.classList.toggle('is-issued', isIssued);
          item.classList.toggle('is-next', isNext);
          item.classList.toggle('is-just', isIssued && s === info.issueCount - 1 && status === 'issue');
        }

        if (status === 'wait') {
          statusEls[w].bar.classList.add('is-visible');
          var ratio = Math.max(0, Math.min(1, info.remaining / latency));
          statusEls[w].fill.style.width = Math.round(ratio * 100) + '%';
        } else {
          statusEls[w].bar.classList.remove('is-visible');
          statusEls[w].fill.style.width = '0%';
        }
      }
    }

    function updateArrow() {
      if (state.elapsed <= 0 || state.lastLaunch < 0) {
        arrow.classList.remove('is-visible');
        return;
      }
      var row = rowEls[state.lastLaunch];
      if (!row) {
        arrow.classList.remove('is-visible');
        return;
      }
      var stageRect = stage.getBoundingClientRect();
      var rowRect = row.getBoundingClientRect();
      var schedRect = sched.getBoundingClientRect();
      var top = rowRect.top - stageRect.top + rowRect.height / 2;
      var bottom = schedRect.top - stageRect.top;
      arrow.style.top = top + 'px';
      arrow.style.height = Math.max(0, bottom - top) + 'px';
      arrow.style.setProperty('--warp-color', colorVar(state.lastLaunch));
      arrow.classList.add('is-visible');
    }

    function updateNotes() {
      for (var i = 0; i < noteEls.length; i++) {
        noteEls[i].classList.toggle('is-current', Number(noteEls[i].dataset.warps) === state.warps);
      }
    }

    function statsText(sim) {
      var launches = sim.launches.slice(0, state.elapsed);
      var issued = 0;
      for (var i = 0; i < launches.length; i++) if (launches[i] >= 0) issued += 1;
      var idle = state.elapsed - issued;
      var utilization = state.elapsed ? Math.round((issued / state.elapsed) * 100) : 0;
      return {
        issued: issued,
        idle: idle,
        utilization: utilization,
        text: '已发射 ' + issued + ' 条 / 空闲 ' + idle + ' 个周期 / 利用率 ' + utilization + '%'
      };
    }

    function render(notify) {
      var sim = simulate(program, latency, state.warps, cycles);
      var activeCycle = state.elapsed === 0 ? 0 : state.elapsed - 1;
      state.lastLaunch = activeCycle < state.elapsed ? sim.launches[activeCycle] : -1;
      state.lastInstruction = activeCycle < state.elapsed ? sim.instructions[activeCycle] : null;

      updateWarpButtons();
      updateTimeline(sim, activeCycle);
      updateRows(sim);
      updateNotes();
      cycleNow.textContent = String(state.elapsed);

      var statsInfo = statsText(sim);
      stats.textContent = statsInfo.text;

      if (state.elapsed === 0) {
        schedState.textContent = '待命';
        schedBox.classList.remove('is-idle');
      } else if (state.lastLaunch < 0) {
        schedState.textContent = '空闲';
        schedBox.classList.add('is-idle');
      } else {
        schedState.textContent = '发射 warp ' + state.lastLaunch;
        schedBox.classList.remove('is-idle');
      }

      window.requestAnimationFrame(updateArrow);

      if (notify) {
        if (state.elapsed === 0) {
          announce('第 0 周期，等待开始；' + state.warps + ' 个 warp，' + statsInfo.text);
        } else if (state.lastLaunch < 0) {
          announce('第 ' + state.elapsed + ' / ' + cycles + ' 周期，调度器空闲；' + statsInfo.text);
        } else {
          announce(
            '第 ' + state.elapsed + ' / ' + cycles + ' 周期，发射 warp ' + state.lastLaunch +
            (state.lastInstruction === 'L' ? '，读显存' : '，计算') + '；' + statsInfo.text
          );
        }
      }
    }

    function finishIfDone() {
      if (state.elapsed < cycles) return false;
      setPlaying(false, false);
      render(false);
      announce('播放结束：' + statsText(simulate(program, latency, state.warps, cycles)).text);
      return true;
    }

    function tickPlay() {
      if (!state.playing) return;
      if (finishIfDone()) return;
      state.elapsed += 1;
      render(false);
      if (state.elapsed >= cycles) {
        finishIfDone();
        return;
      }
      state.timer = window.setTimeout(tickPlay, 250);
    }

    function play() {
      if (state.playing) {
        setPlaying(false, true);
        return;
      }
      if (state.elapsed >= cycles) state.elapsed = 0;
      if (reducedMotion()) {
        state.elapsed = cycles;
        setPlaying(false, false);
        render(false);
        announce('已按“减少动态效果”直接跳到第 ' + cycles + ' 周期；' +
          statsText(simulate(program, latency, state.warps, cycles)).text);
        return;
      }
      setPlaying(true, true);
      tickPlay();
    }

    function step() {
      if (state.playing) setPlaying(false, false);
      if (state.elapsed >= cycles) return;
      state.elapsed += 1;
      render(true);
    }

    function reset() {
      setPlaying(false, false);
      state.elapsed = 0;
      render(true);
    }

    playButton.addEventListener('click', play);
    stepButton.addEventListener('click', step);
    resetButton.addEventListener('click', reset);

    pickerButtons.addEventListener('keydown', function (event) {
      if (event.key !== 'ArrowLeft' && event.key !== 'ArrowRight' &&
          event.key !== 'Home' && event.key !== 'End') return;
      var target = event.target;
      if (!target || !target.classList || !target.classList.contains('cu-warpsched-warpbtn')) return;
      event.preventDefault();
      event.stopPropagation();
      var index = 0;
      for (var i = 0; i < warpButtons.length; i++) if (warpButtons[i] === target) index = i;
      if (event.key === 'ArrowLeft') index = (index - 1 + warpButtons.length) % warpButtons.length;
      else if (event.key === 'ArrowRight') index = (index + 1) % warpButtons.length;
      else if (event.key === 'Home') index = 0;
      else if (event.key === 'End') index = warpButtons.length - 1;
      setWarpCount(Number(warpButtons[index].dataset.warps), true);
    });

    // 点按钮时也把焦点留在按钮上，方便键盘用户继续操作。
    for (var wi = 0; wi < warpButtons.length; wi++) {
      warpButtons[wi].addEventListener('click', function (event) {
        event.currentTarget.focus();
      });
    }

    var resizeTimer = null;
    function queueArrow() {
      if (resizeTimer != null) window.cancelAnimationFrame(resizeTimer);
      resizeTimer = window.requestAnimationFrame(updateArrow);
    }
    if (typeof ResizeObserver === 'function') {
      var ro = new ResizeObserver(queueArrow);
      ro.observe(stage);
    } else {
      window.addEventListener('resize', queueArrow);
    }

    function beforePrint() {
      if (state.printSnapshot) return;
      state.printSnapshot = { warps: state.warps, elapsed: state.elapsed };
      setPlaying(false, false);
      state.warps = warpChoices.indexOf(8) >= 0 ? 8 : warpChoices[warpChoices.length - 1];
      state.elapsed = cycles;
      updateWarpButtons();
      buildRows();
      render(false);
      figure.classList.add('is-printing');
    }

    function afterPrint() {
      if (!state.printSnapshot) return;
      state.warps = state.printSnapshot.warps;
      state.elapsed = state.printSnapshot.elapsed;
      state.printSnapshot = null;
      figure.classList.remove('is-printing');
      updateWarpButtons();
      buildRows();
      render(false);
    }
    window.addEventListener('beforeprint', beforePrint);
    window.addEventListener('afterprint', afterPrint);

    updateWarpButtons();
    buildRows();
    render(false);
    figure.classList.add('cu-warpsched-ready');
  }

  // ---------------------------------------------------------------------------
  // 导出与自动初始化
  // ---------------------------------------------------------------------------

  var api = {
    threadPartition: threadPartition,
    threadToWarp: threadToWarp,
    threadToLane: threadToLane,
    laneRowsForWidth: laneRowsForWidth,
    formatThreadReadout: formatThreadReadout,
    normalizeProgram: normalizeProgram,
    simulate: simulate
  };

  if (typeof module !== 'undefined' && module.exports) {
    module.exports = api;
  }

  if (typeof document !== 'undefined') {
    var start = function () {
      var figures = document.querySelectorAll('figure.cu-warp');
      for (var i = 0; i < figures.length; i++) initWarpFigure(figures[i]);
      var schedFigures = document.querySelectorAll('figure.cu-warpsched');
      for (var j = 0; j < schedFigures.length; j++) initWarpschedFigure(schedFigures[j]);
    };
    if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', start);
    else start();
  }
})();
