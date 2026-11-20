/* CMake 副作用查看器。
 *
 * 数据是 tools/sidefx/record.py 录下的一次真实运行（JSON）：cmake 在自己的调试器下逐条命令停下，
 * 每次停下都记下调用栈、各层作用域的变量、缓存、目标和工作目录里的文件。
 * 这里把相邻两次停下之间的差别，当成这一条命令的副作用显示出来。
 *
 * 页面里：<div class="sfx-launch" data-src="sidefx/ch03/x.json"> 里的 .sfx-open 按钮打开弹出层；
 * 独立页面：window.SIDEFX_DATA 存在时直接铺满整页。
 */
(function () {
    'use strict';

    // ------------------------------------------------------------------ 工具

    var ESC = { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' };
    function esc(s) { return String(s == null ? '' : s).replace(/[&<>"]/g, function (c) { return ESC[c]; }); }

    function h(tag, attrs) {
        var e = document.createElement(tag);
        if (attrs) {
            for (var k in attrs) {
                if (!Object.prototype.hasOwnProperty.call(attrs, k) || attrs[k] == null || attrs[k] === false) continue;
                if (k === 'class') e.className = attrs[k];
                else if (k === 'html') e.innerHTML = attrs[k];
                else if (k === 'text') e.textContent = attrs[k];
                else if (k.slice(0, 2) === 'on') e.addEventListener(k.slice(2), attrs[k]);
                else e.setAttribute(k, attrs[k] === true ? '' : attrs[k]);
            }
        }
        for (var i = 2; i < arguments.length; i++) {
            var c = arguments[i];
            if (c == null || c === false) continue;
            if (Array.isArray(c)) c.forEach(function (x) { if (x != null && x !== false) e.appendChild(typeof x === 'string' ? document.createTextNode(x) : x); });
            else e.appendChild(typeof c === 'string' ? document.createTextNode(c) : c);
        }
        return e;
    }

    function applyChanges(obj, changes) {
        var o = Object.assign({}, obj);
        (changes || []).forEach(function (c) {
            if (c.op === 'del') delete o[c.name];
            else o[c.name] = c.new;
        });
        return o;
    }

    function fmtSize(n) {
        if (n == null) return '';
        if (n < 1024) return n + ' B';
        if (n < 1024 * 1024) return (n / 1024).toFixed(n < 10240 ? 1 : 0) + ' KB';
        return (n / 1048576).toFixed(1) + ' MB';
    }

    function short(v, n) {
        v = String(v == null ? '' : v);
        n = n || 160;
        return v.length > n ? v.slice(0, n - 1) + '…' : v;
    }

    var PROP = {
        '@IncludeDirectories': 'INCLUDE_DIRECTORIES',
        '@CompileDefinitions': 'COMPILE_DEFINITIONS',
        '@CompileOptions': 'COMPILE_OPTIONS',
        '@CompileFeatures': 'COMPILE_FEATURES',
        '@LinkImplementations': 'LINK_LIBRARIES',
        '@LinkOptions': 'LINK_OPTIONS',
        '@LinkDirectories': 'LINK_DIRECTORIES',
        '@PrecompileHeaders': 'PRECOMPILE_HEADERS',
        '@Sources': 'SOURCES',
        '@LinkInterfaces': 'INTERFACE_LINK_LIBRARIES'
    };
    function propName(k) { return PROP[k] || k.replace(/^@/, ''); }

    var TYPE_ZH = {
        STATIC_LIBRARY: '静态库', SHARED_LIBRARY: '动态库', MODULE_LIBRARY: '模块库',
        OBJECT_LIBRARY: '对象库', INTERFACE_LIBRARY: '接口库', EXECUTABLE: '可执行文件',
        UTILITY: '自定义目标', UNKNOWN_LIBRARY: '导入的库（类型未知）', GLOBAL_TARGET: '全局目标', ALIAS: '别名'
    };
    function typeZh(t) { return TYPE_ZH[t] ? TYPE_ZH[t] + ' ' + t : t; }

    // CMake 自己在每个作用域里放的变量：默认折叠
    function isBuiltinVar(name) {
        return /^(CMAKE_|_|ARGV\d|ARGC$|ARGN$|ARGV$|PROJECT_|MSVC|UNIX$|WIN32$|APPLE$|LINUX$|BSD$|CYGWIN$|MINGW$|ANDROID$|IOS$|XCODE|GHSMULTI|BORLAND|WATCOM)/.test(name) ||
            /^(CMAKE|_CMAKE)/.test(name);
    }

    // ------------------------------------------------------------------ CMake 语法着色

    function hlVars(s) {
        // s 已转义
        return s.replace(/(\$(?:ENV|CACHE)?\{[^}]*\})/g, '<span class="k-var">$1</span>')
            .replace(/(\$&lt;[^&]*?(?:&gt;|$))/g, '<span class="k-gex">$1</span>');
    }

    function highlight(line) {
        var out = '', i = 0, n = line.length;
        var m = /^(\s*)([A-Za-z_][A-Za-z0-9_]*)(\s*\()/.exec(line);
        if (m) {
            out += esc(m[1]) + '<span class="k-cmd">' + esc(m[2]) + '</span>' + esc(m[3]);
            i = m[0].length;
        }
        var buf = '';
        function flush() { if (buf) { out += hlVars(esc(buf)); buf = ''; } }
        while (i < n) {
            var c = line[i];
            if (c === '#') { flush(); out += '<span class="k-com">' + esc(line.slice(i)) + '</span>'; return out; }
            if (c === '"') {
                flush();
                var j = i + 1;
                while (j < n && line[j] !== '"') { if (line[j] === '\\') j++; j++; }
                out += '<span class="k-str">' + hlVars(esc(line.slice(i, j + 1))) + '</span>';
                i = j + 1;
                continue;
            }
            buf += c;
            i++;
        }
        flush();
        return out;
    }

    // ------------------------------------------------------------------ 数据模型

    function Run(rec, index) {
        this.rec = rec;
        this.index = index;
        this.run = rec.runs[index];
        this.cfg = this.run.configure || {};
        this.steps = this.cfg.steps || [];
        this.srcRoot = this.run.src || 'src';
        var srcs = null;
        for (var i = index; i >= 0 && !srcs; i--) srcs = rec.runs[i].sources;
        this.sources = {};
        var self = this;
        (srcs || []).forEach(function (f) { self.sources[f.path] = f; });
        this.computeStops();
        this.lineSteps = {};
        this.steps.forEach(function (s, k) {
            var key = s.file + ':' + s.line;
            (self.lineSteps[key] = self.lineSteps[key] || []).push(k);
        });
        this.files = [];
        this.steps.forEach(function (s) { if (self.files.indexOf(s.file) < 0) self.files.push(s.file); });
        // CMake 自己放进作用域的变量：第一行之前就有的，以及 project()/enable_language() 里设置的。
        // project() 设置的 PROJECT_* 和 <项目名>_* 是给用户看的，不算。
        this.own = {};
        Object.keys(((this.cfg.state0 || {}).scopes || [{}])[0] || {}).forEach(function (n) { self.own[n] = 1; });
        var projNames = [];
        this.steps.forEach(function (s) {
            if (s.cmd === 'project' && s.args && s.args[0]) projNames.push(s.args[0]);
            if (s.cmd !== 'project' && s.cmd !== 'enable_language') return;
            ((s.fx || {}).vars || []).forEach(function (v) {
                v.changes.forEach(function (c) {
                    if (c.op !== 'add') return;
                    if (/^PROJECT_/.test(c.name) || projNames.some(function (p) { return c.name.indexOf(p + '_') === 0; })) return;
                    self.own[c.name] = 1;
                });
            });
        });
    }

    Run.prototype.builtin = function (name) { return !!this.own[name] || isBuiltinVar(name); };

    Run.prototype.blob = function (key) { return key ? this.rec.blobs[key] : null; };

    Run.prototype.sourceLines = function (file) {
        var f = this.sources[file];
        var text = f ? this.blob(f.blob) : null;
        if (text == null) return [];
        var lines = text.split('\n');
        if (lines.length && lines[lines.length - 1] === '') lines.pop();
        return lines;
    };

    Run.prototype.computeStops = function () {
        var st0 = this.cfg.state0 || {};
        var scope = Object.assign({}, (st0.scopes || [{}])[0]);
        var cache = Object.assign({}, st0.cache || {});
        var targets = {};
        Object.keys(st0.targets || {}).forEach(function (n) { targets[n] = st0.targets[n]; });
        var files = {};
        (st0.files || []).forEach(function (f) { files[f.path] = f; });
        this.stops = [];
        var steps = this.steps;
        for (var k = 0; k < steps.length; k++) {
            if (k > 0) {
                var fx = steps[k - 1].fx || {};
                if (steps[k].scope) scope = applyChanges(scope, steps[k].scope);
                else if (!fx.entered && !fx.left) {
                    var top = (fx.vars || []).filter(function (v) { return v.top; })[0];
                    if (top) scope = applyChanges(scope, top.changes);
                }
                if (fx.cache) cache = applyChanges(cache, fx.cache);
                if (fx.targets) targets = applyTargets(targets, fx.targets);
                if (fx.files) files = applyFiles(files, fx.files);
            }
            this.stops.push({ scope: scope, cache: cache, targets: targets, files: files });
        }
        var last = this.stops[this.stops.length - 1] || { scope: scope, cache: cache, targets: targets, files: files };
        this.afterConfigure = last;
        this.afterGenerate = Object.assign({}, last, { files: applyFiles(last.files, (this.cfg.generate || {}).files) });
        var b = this.run.build_result;
        this.afterBuild = b ? Object.assign({}, last, { files: applyFiles(this.afterGenerate.files, b.files) }) : this.afterGenerate;
    };

    Run.prototype.after = function (k) { return this.stops[Math.min(k + 1, this.stops.length - 1)]; };

    function applyTargets(targets, changes) {
        var t = Object.assign({}, targets);
        changes.forEach(function (c) {
            if (c.op === 'add') t[c.name] = { type: c.type, props: c.props || {}, dir: c.dir, imported: c.imported, alias: c.alias,
                fromTrace: c.fromTrace, born: c.fromTrace ? {} : (c.props || {}) };
            else if (c.op === 'set' && t[c.name]) t[c.name] = Object.assign({}, t[c.name], { props: applyChanges(t[c.name].props, c.changes) });
        });
        return t;
    }

    function applyFiles(files, changes) {
        var f = Object.assign({}, files);
        (changes || []).forEach(function (c) {
            if (c.op === 'del') delete f[c.path];
            else f[c.path] = c;
        });
        return f;
    }

    // 回溯（file API 的 backtrace）里的文件名 → 录制里的路径
    Run.prototype.btKey = function (file) {
        if (!file) return '';
        if (file.indexOf('/work/') === 0) return file.slice(6);
        if (file[0] === '/') return file;
        return this.srcRoot + '/' + file;
    };

    Run.prototype.display = function (path) {
        if (!path) return '';
        var p = this.srcRoot + '/';
        if (path.indexOf(p) === 0) return path.slice(p.length);
        return path;
    };

    Run.prototype.inSource = function (path) { return path.indexOf(this.srcRoot + '/') === 0; };
    var IN_SRC = ' · 注意：写进了源码目录';

    Run.prototype.firstStep = function (file, line) {
        var l = this.lineSteps[file + ':' + line];
        return l ? l[0] : -1;
    };

    Run.prototype.commandText = function (step) {
        var lines = this.sourceLines(step.file);
        var end = step.line_end || step.line;
        var txt = lines.slice(step.line - 1, end).join('\n');
        return txt.replace(/^\s+/, '');
    };

    // ------------------------------------------------------------------ 弹出层

    function Viewer(rec, opts) {
        this.rec = rec;
        this.opts = opts || {};
        this.runIndex = 0;
        this.phase = 'configure';
        this.k = 0;
        this.fileTab = null;
        this.stateTab = 'vars';
        this.hideBuiltin = true;
        this.hideAdvanced = true;
        this.hideDefaultProps = true;
        this.filter = '';
        this.timer = null;
        this.runs = rec.runs.map(function (_, i) { return new Run(rec, i); });
        this.build();
    }

    Viewer.prototype.build = function () {
        var self = this;
        this.overlay = h('div', { class: 'sfx-overlay', role: 'dialog', 'aria-modal': 'true',
            onclick: function (e) { if (e.target === self.overlay && !self.opts.standalone) self.close(); } });
        this.modal = h('div', { class: 'sfx-modal' });
        this.overlay.appendChild(this.modal);
        this.head = h('div', { class: 'sfx-head' });
        this.bodyEl = h('div', { class: 'sfx-body' });
        this.modal.appendChild(this.head);
        this.modal.appendChild(this.bodyEl);
        this.onKey = function (e) { self.key(e); };
        document.addEventListener('keydown', this.onKey, true);
        (this.opts.mount || document.body).appendChild(this.overlay);
        if (!this.opts.standalone) {
            this.savedOverflow = document.body.style.overflow;
            document.body.style.overflow = 'hidden';
        }
        if (this.opts.run) this.runIndex = Math.min(this.opts.run, this.runs.length - 1);
        if (this.opts.phase) this.phase = this.opts.phase;
        else if (this.R().cfg.skipped) this.phase = 'build';
        if (this.opts.step != null) this.k = this.opts.step;
        this.render();
        this.overlay.tabIndex = -1;
        this.overlay.focus();
    };

    Viewer.prototype.close = function () {
        this.stop();
        document.removeEventListener('keydown', this.onKey, true);
        if (this.overlay.parentNode) this.overlay.parentNode.removeChild(this.overlay);
        if (!this.opts.standalone) document.body.style.overflow = this.savedOverflow || '';
        if (this.opts.returnFocus) this.opts.returnFocus.focus();
    };

    Viewer.prototype.key = function (e) {
        // mdBook 在 document 上监听左右方向键翻页：弹出层打开时先截下来
        e.stopPropagation();
        var t = e.target;
        if (t && (t.tagName === 'INPUT' || t.tagName === 'TEXTAREA')) {
            if (e.key === 'Escape') t.blur();
            return;
        }
        if (e.key === 'Escape') {
            if (this.fileView) { this.closeFile(); e.preventDefault(); return; }
            if (!this.opts.standalone) { this.close(); e.preventDefault(); }
            return;
        }
        if (this.phase !== 'configure') return;
        if (e.key === 'ArrowRight' || e.key === 'j') { this.go(this.k + 1); e.preventDefault(); }
        else if (e.key === 'ArrowLeft' || e.key === 'k') { this.go(this.k - 1); e.preventDefault(); }
        else if (e.key === 'Home') { this.go(0); e.preventDefault(); }
        else if (e.key === 'End') { this.go(this.R().steps.length - 1); e.preventDefault(); }
        else if (e.key === ' ') { this.toggle(); e.preventDefault(); }
    };

    Viewer.prototype.R = function () { return this.runs[this.runIndex]; };

    Viewer.prototype.go = function (k) {
        var n = this.R().steps.length;
        if (!n) return;
        k = Math.max(0, Math.min(n - 1, k));
        if (k === this.k && this.phase === 'configure') return;
        this.k = k;
        this.phase = 'configure';
        this.renderConfigure();
    };

    Viewer.prototype.toggle = function () { if (this.timer) this.stop(); else this.play(); };
    Viewer.prototype.play = function () {
        var self = this;
        if (this.k >= this.R().steps.length - 1) this.k = -1;
        this.timer = setInterval(function () {
            if (self.k >= self.R().steps.length - 1) { self.stop(); return; }
            self.go(self.k + 1);
        }, 1100);
        this.renderControls();
    };
    Viewer.prototype.stop = function () {
        if (this.timer) clearInterval(this.timer);
        this.timer = null;
        if (this.ctl) this.renderControls();
    };

    Viewer.prototype.render = function () {
        this.renderHead();
        if (this.phase === 'configure') this.renderConfigure(true);
        else this.renderPhase();
    };

    Viewer.prototype.renderHead = function () {
        var self = this, R = this.R(), rec = this.rec;
        this.head.innerHTML = '';
        this.head.appendChild(h('span', { class: 'sfx-title', text: rec.title || 'CMake 副作用' }));
        if (rec.runs.length > 1) {
            var runs = h('div', { class: 'sfx-tabs sfx-runs' });
            rec.runs.forEach(function (r, i) {
                runs.appendChild(h('button', { class: 'sfx-tab' + (i === self.runIndex ? ' on' : ''),
                    onclick: function () { self.stop(); self.runIndex = i; self.k = 0; self.fileTab = null; self.phase = self.runs[i].cfg.skipped ? 'build' : 'configure'; self.render(); } },
                    '①②③④⑤⑥⑦⑧⑨'[i] + ' ' + (r.label || ('第 ' + (i + 1) + ' 次'))));
            });
            this.head.appendChild(runs);
            this.head.appendChild(h('span', { class: 'sfx-sep' }));
        }
        var phases = h('div', { class: 'sfx-tabs' });
        var list = [];
        if (!R.cfg.skipped) {
            list.push(['configure', '配置', R.steps.length + ' 步']);
            list.push(['generate', '生成', ((R.cfg.generate || {}).files || []).length + ' 个文件']);
        }
        if (R.run.build_result) list.push(['build', '构建', (R.run.build_result.commands || []).length + ' 条命令']);
        if (R.run.after) list.push(['after', '运行', R.run.after.length + ' 条']);
        list.forEach(function (p) {
            phases.appendChild(h('button', { class: 'sfx-tab' + (self.phase === p[0] ? ' on' : ''), 'data-phase': p[0],
                onclick: function () { self.stop(); self.phase = p[0]; self.render(); } },
                p[1], h('span', { class: 'n', text: p[2] })));
        });
        this.head.appendChild(phases);
        if (!this.opts.standalone) {
            this.head.appendChild(h('button', { class: 'sfx-close', title: '关闭（Esc）', 'aria-label': '关闭', onclick: function () { self.close(); } }, '×'));
        }
        var argv = R.cfg.argv ? R.cfg.argv.join(' ') : '';
        if (this.phase === 'build' && R.run.build_result) argv = R.run.build_result.argv.join(' ');
        if (R.cfg.skipped && this.phase !== 'build') argv = '';
        if (argv && this.phase !== 'after') this.head.appendChild(h('div', { class: 'sfx-argv', text: argv }));
        if (R.run.replaced && R.run.replaced.length && this.phase === 'configure') {
            this.head.appendChild(h('div', { class: 'sfx-note', text: '这次运行之前改了：' + R.run.replaced.map(function (p) { return R.display(p); }).join('、') }));
        }
    };

    // ------------------------------------------------------------------ 配置阶段

    Viewer.prototype.renderConfigure = function (full) {
        var R = this.R();
        if (full || !this.leftEl) {
            this.bodyEl.innerHTML = '';
            this.leftEl = h('div', { class: 'sfx-left' });
            this.rightEl = h('div', { class: 'sfx-right' });
            this.bodyEl.appendChild(this.leftEl);
            this.bodyEl.appendChild(this.rightEl);
            this.filesEl = h('div', { class: 'sfx-files' });
            this.stackEl = h('div', { class: 'sfx-stack' });
            this.codeWrap = h('div', { class: 'sfx-scroll' });
            this.ctl = h('div', { class: 'sfx-ctl' });
            this.leftEl.appendChild(this.filesEl);
            this.leftEl.appendChild(this.stackEl);
            this.leftEl.appendChild(this.codeWrap);
            this.leftEl.appendChild(this.ctl);
            this.nowEl = h('div', { class: 'sfx-now sfx-pad' });
            this.stateEl = h('div', { class: 'sfx-state' });
            this.rightEl.appendChild(this.nowEl);
            this.rightEl.appendChild(this.stateEl);
            this.renderHead();
        }
        if (!R.steps.length) {
            this.codeWrap.innerHTML = '';
            this.nowEl.innerHTML = '';
            this.nowEl.appendChild(h('div', { class: 'sfx-none', text: '这次运行没有执行到项目里的命令。' }));
            this.renderOutputOnly();
            return;
        }
        var step = R.steps[this.k];
        if (!this.fileTab || this.lastK !== this.k) this.fileTab = step.file;
        this.lastK = this.k;
        this.renderFileTabs();
        this.renderStack(step);
        this.renderCode();
        this.renderControls();
        this.renderNow(step);
        this.renderState();
    };

    Viewer.prototype.renderOutputOnly = function () {
        var R = this.R();
        this.stateEl.innerHTML = '';
        var out = (R.cfg.output || []).concat(R.cfg.tail || []);
        this.stateEl.appendChild(h('div', { class: 'sfx-pad' }, h('div', { class: 'sfx-out', html: colorOut(out) })));
    };

    Viewer.prototype.renderFileTabs = function () {
        var self = this, R = this.R();
        this.filesEl.innerHTML = '';
        R.files.forEach(function (f) {
            self.filesEl.appendChild(h('button', { class: 'sfx-tab' + (f === self.fileTab ? ' on' : ''),
                onclick: function () { self.fileTab = f; self.renderFileTabs(); self.renderCode(); } }, R.display(f)));
        });
    };

    Viewer.prototype.renderStack = function (step) {
        var R = this.R();
        this.stackEl.innerHTML = '';
        var parts = (step.stack || []).map(function (f) {
            return h('code', { text: R.display(f.file) + ':' + f.line + ' ' + f.name + '()' });
        });
        var label = step.scopeLabel ? scopeName(step.scopeLabel, R) : '';
        if (!parts.length) {
            this.stackEl.appendChild(h('span', { text: '调用栈：顶层 · 当前作用域：' + label }));
            return;
        }
        this.stackEl.appendChild(document.createTextNode('调用栈：'));
        parts.forEach(function (p) { this.stackEl.appendChild(p); this.stackEl.appendChild(h('span', { class: 'arrow', text: '→' })); }, this);
        this.stackEl.appendChild(h('code', { text: R.display(step.file) + ':' + step.line }));
        this.stackEl.appendChild(document.createTextNode(' · 当前作用域：' + label));
    };

    function scopeName(s, R) {
        if (!s) return '';
        if (s.kind === 'function') return '函数 ' + s.name + '()';
        var d = R ? R.display(s.name) : s.name;
        if (d === 'src' || d === R.srcRoot || d === '' || d === '.') return '顶层目录';
        return '目录 ' + d + '/';
    }

    Viewer.prototype.renderCode = function () {
        var self = this, R = this.R();
        var file = this.fileTab, step = R.steps[this.k];
        var lines = R.sourceLines(file);
        var hits = {};
        for (var j = 0; j <= this.k; j++) {
            var s = R.steps[j];
            if (s.file === file) hits[s.line] = (hits[s.line] || 0) + 1;
        }
        var curFrom = -1, curTo = -1;
        if (step.file === file) { curFrom = step.line; curTo = step.line_end || step.line; }
        // 调用栈里外层的行（例如调用函数的那一行）也标出来
        var outer = {};
        (step.stack || []).forEach(function (f) { if (f.file === file) outer[f.line] = true; });
        var pre = h('div', { class: 'sfx-code' });
        var curEl = null;
        var n = lines.length;
        var sentinelHere = R.steps.some(function (s) { return s.sentinel && s.file === file; });
        var total = n + (sentinelHere ? 1 : 0);
        for (var i = 1; i <= total; i++) {
            var isEnd = i > n;
            var cls = 'sfx-ln';
            if (i >= curFrom && i <= curTo) cls += ' cur';
            var clickable = !!R.lineSteps[file + ':' + i];
            if (clickable) cls += ' click';
            if (isEnd) cls += ' end';
            var tx = isEnd ? (R.display(file) === 'CMakeLists.txt' && R.srcRoot === file.replace(/\/CMakeLists\.txt$/, '')
                ? '⏎ 顶层 CMakeLists.txt 执行完：配置阶段到此结束'
                : '⏎ 文件执行完：这个目录的作用域到此结束') : null;
            var row = h('div', { class: cls, 'data-line': i },
                h('span', { class: 'no', text: isEnd ? '' : String(i) }),
                h('span', { class: 'hits', text: hits[i] ? (hits[i] > 1 ? '×' + hits[i] : '●') : (outer[i] ? '↳' : '') }),
                h('span', { class: 'tx', html: isEnd ? esc(tx) : (highlight(lines[i - 1]) || ' ') }));
            if (clickable) {
                (function (line) {
                    row.addEventListener('click', function () {
                        var l = R.lineSteps[file + ':' + line];
                        var next = l.filter(function (x) { return x > self.k; })[0];
                        self.stop();
                        self.go(next != null ? next : l[0]);
                    });
                })(i);
            }
            if (i === curFrom) curEl = row;
            pre.appendChild(row);
        }
        this.codeWrap.innerHTML = '';
        this.codeWrap.appendChild(pre);
        if (curEl) {
            var wrap = this.codeWrap;
            var top = curEl.offsetTop - wrap.clientHeight / 3;
            wrap.scrollTop = Math.max(0, top);
        }
    };

    Viewer.prototype.renderControls = function () {
        var self = this, R = this.R(), n = R.steps.length;
        if (!this.ctl) return;
        this.ctl.innerHTML = '';
        this.ctl.appendChild(h('button', { class: 'sfx-btn', title: '第一步（Home）', disabled: this.k <= 0, onclick: function () { self.stop(); self.go(0); } }, '⏮'));
        this.ctl.appendChild(h('button', { class: 'sfx-btn', title: '上一步（←）', disabled: this.k <= 0, onclick: function () { self.stop(); self.go(self.k - 1); } }, '◀'));
        this.ctl.appendChild(h('button', { class: 'sfx-btn primary', title: '播放 / 暂停（空格）', onclick: function () { self.toggle(); } }, this.timer ? '❚❚' : '▶'));
        this.ctl.appendChild(h('button', { class: 'sfx-btn', title: '下一步（→）', disabled: this.k >= n - 1, onclick: function () { self.stop(); self.go(self.k + 1); } }, '▶▏'));
        var range = h('input', { type: 'range', min: 0, max: Math.max(0, n - 1), value: this.k, 'aria-label': '步数',
            oninput: function () { self.stop(); self.go(parseInt(range.value, 10)); } });
        this.ctl.appendChild(range);
        this.ctl.appendChild(h('span', { class: 'sfx-pos', text: '第 ' + (this.k + 1) + ' / ' + n + ' 步' }));
    };

    // ---------- 这一步的副作用

    Viewer.prototype.renderNow = function (step) {
        var self = this, R = this.R(), fx = step.fx || {};
        var el = this.nowEl;
        el.innerHTML = '';
        var txt = step.sentinel ? '' : R.commandText(step);
        var cmd = h('div', { class: 'sfx-cmdline' });
        cmd.appendChild(h('span', { class: 'lab', text: '第 ' + (this.k + 1) + ' 步 · ' + R.display(step.file) + ':' + step.line + (step.sentinel ? '（文件末尾）' : '') }));
        if (step.sentinel) cmd.appendChild(document.createTextNode(isTopFile(R, step.file) ? '顶层 CMakeLists.txt 执行完毕' : R.display(step.file) + ' 执行完毕，回到调用它的地方'));
        else cmd.appendChild(h('span', { html: highlight(txt) }));
        el.appendChild(cmd);
        if (!step.sentinel && step.args && needsExpanded(txt)) {
            el.appendChild(h('div', { class: 'sfx-note' }, '展开变量后，命令实际收到的参数：',
                h('div', { class: 'sfx-cmdline', text: step.cmd + '(' + step.args.map(quoteArg).join(' ') + ')' })));
        }
        var any = false;
        function section(title, sub, items) {
            if (!items || !items.length) return;
            any = true;
            el.appendChild(h('div', { class: 'sfx-h' }, title, sub ? h('span', { class: 'sub', text: sub }) : null));
            el.appendChild(items.length > 14 ? collapsible(items, 10) : h('ul', { class: 'sfx-items' }, items));
        }

        // 作用域：进入 / 离开
        (fx.entered || []).forEach(function (e) {
            var items = [];
            var lab = scopeName(e.scope, R);
            var what;
            if (e.via === 'add_subdirectory') what = 'add_subdirectory() 进入 ' + R.display(e.file) + '，新建' + lab + '的作用域：先复制父目录的全部变量';
            else if (e.scope.kind === 'function' && e.via === e.scope.name) what = '调用函数 ' + e.scope.name + '()：新建函数作用域，先复制调用者的全部变量，再放入参数';
            else if (e.via === 'include') what = 'include() 读入 ' + R.display(e.file) + '：在当前作用域里执行，不新建作用域';
            else if (e.via === 'find_package') what = 'find_package() 读入 ' + R.display(e.file) + '：在当前作用域里执行';
            else what = '进入 ' + R.display(e.file) + '（' + (e.via || '') + '）：在调用者的作用域里执行（宏或 include 不新建作用域）';
            items.push(item('info', '→', what));
            (e.changes || []).forEach(function (c) { items.push(varItem(c)); });
            section('作用域', lab, items);
        });
        (fx.dropped || []).forEach(function (d) {
            var names = d.names.filter(function (n) { return !/^CMAKE_CURRENT_/.test(n); });
            var lab = scopeName(d.scope, R);
            var items = [item('del', '←', '离开' + lab + '：这个作用域连同里面的变量一起销毁' +
                (names.length ? '，' + names.length + ' 个只在这里有的变量消失：' : ''),
                names.length ? h('code', { text: names.slice(0, 12).join('  ') + (names.length > 12 ? '  …' : '') }) : null)];
            section('作用域', lab, items);
        });
        if ((fx.left || []).length && !(fx.dropped || []).length && !(fx.entered || []).length) {
            section('作用域', '', [item('info', '←', '回到调用者：' + fx.left.map(function (l) { return R.display(l.file); }).join('、') + ' 执行完（没有新建作用域，变量都留在调用者里）')]);
        }

        // 变量
        (fx.vars || []).forEach(function (v) {
            var shown = v.changes.filter(function (c) { return !NOISE[c.name]; });
            if (!shown.length) return;
            var items = sortBuiltinLast(shown, R).map(varItem);
            var sub = scopeName(v.scope, R);
            if (!v.top) sub += (step.cmd === 'set' && /PARENT_SCOPE/.test(txt)) ? '（PARENT_SCOPE：写到了上一层）' : '（外层作用域）';
            section('变量', sub, items);
        });

        // 缓存
        if (fx.cache) {
            section('缓存', '全局，会写进 CMakeCache.txt', fx.cache.map(function (c) {
                if (c.op === 'add') return item('add', '+', h('code', { text: c.name }), ' ', h('span', { class: 'meta', text: c.new.type }), ' = ', h('code', { text: short(c.new.value) }),
                    c.new.help ? h('div', { class: 'meta', text: c.new.help }) : null);
                if (c.op === 'del') return item('del', '−', h('code', { text: c.name }), ' 从缓存里删除');
                var parts = [];
                if (c.old.value !== c.new.value) parts.push(h('span', {}, h('code', { class: 'old', text: short(c.old.value, 60) }), ' → ', h('code', { text: short(c.new.value) })));
                if (c.old.type !== c.new.type) parts.push(h('span', { class: 'meta', text: ' 类型 ' + c.old.type + ' → ' + c.new.type }));
                if (c.old.help !== c.new.help) parts.push(h('div', { class: 'meta', text: '说明：' + (c.new.help || '（空）') }));
                return item('set', '~', h('code', { text: c.name }), ' ', parts);
            }));
        }

        // 目标
        if (fx.targets) {
            var titems = [];
            fx.targets.forEach(function (t) {
                if (t.op === 'add' && t.alias) {
                    titems.push(item('add', '+', '别名 ', h('code', { text: t.name }), ' → ', h('code', { text: (t.props || {}).ALIASED_TARGET || '' }),
                        h('div', { class: 'meta', text: 'ALIAS 不是新目标，只是同一个目标的另一个名字；用到它的地方都换成原目标' })));
                } else if (t.op === 'add') {
                    var where = t.dir ? ' · 创建于 ' + (R.display(t.dir) === R.srcRoot ? '顶层' : R.display(t.dir) + '/') : '';
                    titems.push(item('add', '+', t.imported ? '导入目标 ' : '新目标 ', h('code', { text: t.name }), ' ', h('span', { class: 'meta', text: typeZh(t.type) + (t.imported ? ' · 导入的' : '') + where }),
                        t.fromTrace ? h('div', { class: 'meta', text: '调试器不列出导入目标，下面的属性是从执行过的命令里还原的' }) : null));
                    var p = t.props || {};
                    Object.keys(p).filter(function (k) { return (k[0] === '@' || t.fromTrace) && p[k]; }).forEach(function (k) {
                        titems.push(item('add', '', h('code', { text: propName(k) }), ' = ', h('code', { text: short(p[k]) })));
                    });
                } else if (t.op === 'set') {
                    t.changes.forEach(function (c) {
                        titems.push(item(c.op === 'add' ? 'add' : c.op === 'del' ? 'del' : 'set', c.op === 'add' ? '+' : c.op === 'del' ? '−' : '~',
                            h('code', { text: t.name }), ' 的 ', h('code', { text: propName(c.name) }), ' ',
                            c.op === 'set' ? h('span', {}, h('code', { class: 'old', text: short(c.old, 80) }), ' → ') : (c.op === 'add' ? '= ' : ''),
                            c.op !== 'del' ? h('code', { text: short(c.new) }) : '被清空'));
                    });
                }
            });
            section('目标', '全局，名字在整个工程里唯一', titems);
        }

        // 文件
        if (fx.files) {
            section('文件', '在磁盘上', groupFiles(fx.files).map(function (f) {
                if (f.group) return item(f.op, f.op === 'add' ? '+' : '~', h('code', { text: R.display(f.dir) + '/' }), ' 下 ' + f.count + ' 个文件', h('span', { class: 'meta', text: '（CMake 自己用的，' + fmtSize(f.size) + '）' + (R.inSource(f.dir) ? IN_SRC : '') }));
                var name = h('code', { class: f.blob ? 'sfx-link' : '', text: R.display(f.path),
                    onclick: f.blob ? function () { self.showFile(f.path, f.blob); } : null });
                return item(f.op === 'del' ? 'del' : f.op, f.op === 'add' ? '+' : f.op === 'del' ? '−' : '~', name, ' ',
                    h('span', { class: 'meta', text: (f.op === 'add' ? '新建' : f.op === 'del' ? '删除' : '改写') + ' · ' + fmtSize(f.size) + (R.inSource(f.path) ? IN_SRC : '') }));
            }));
        }

        // 输出
        if (fx.out && fx.out.some(function (l) { return l; })) {
            any = true;
            el.appendChild(h('div', { class: 'sfx-h', text: '输出' }));
            el.appendChild(h('div', { class: 'sfx-out', html: colorOut(fx.out) }));
        }
        if (step.inner) {
            el.appendChild(h('div', { class: 'sfx-note', text: '这条命令在 CMake 自带的模块里又执行了 ' + step.inner.toLocaleString() + ' 条命令（查看器跳过了它们，只看结果）。' }));
        }
        var note = explain(step, R, this.k, any, txt);
        if (note) el.appendChild(h('div', { class: any ? 'sfx-note' : 'sfx-none', text: note }));
        else if (!any) el.appendChild(h('div', { class: 'sfx-none', text: '没有看得见的副作用。' }));
    };

    // 调试器每次停下时都会变、但和这条命令无关的变量
    var NOISE = { CMAKE_PARENT_LIST_FILE: 1, CMAKE_CURRENT_LIST_LINE: 1 };

    function isTopFile(R, file) { return file === R.srcRoot + '/CMakeLists.txt'; }

    function needsExpanded(txt) { return /\$\{|\$ENV\{|\$CACHE\{|\$</.test(txt) || /\n/.test(txt) && false; }

    function quoteArg(a) {
        if (a === '') return '""';
        return /[\s;()"#]/.test(a) ? '"' + a.replace(/"/g, '\\"') + '"' : a;
    }

    function sortBuiltinLast(changes, R) {
        var a = [], b = [];
        changes.forEach(function (c) { (R.builtin(c.name) ? b : a).push(c); });
        return a.concat(b);
    }

    function item(cls, op, a, b, c, d, e, f) {
        var body = h('div', { class: 'body' });
        [a, b, c, d, e, f].forEach(function (x) {
            if (x == null || x === false) return;
            if (Array.isArray(x)) x.forEach(function (y) { body.appendChild(typeof y === 'string' ? document.createTextNode(y) : y); });
            else body.appendChild(typeof x === 'string' ? document.createTextNode(x) : x);
        });
        return h('li', { class: 'sfx-item ' + cls }, h('span', { class: 'op', text: op }), body);
    }

    function varItem(c) {
        if (c.op === 'add') return item('add', '+', h('code', { text: c.name }), ' = ', h('code', { text: c.new === '' ? '""' : short(c.new) }));
        if (c.op === 'del') return item('del', '−', h('code', { text: c.name }), ' 被删除');
        return item('set', '~', h('code', { text: c.name }), ': ', h('code', { class: 'old', text: short(c.old === '' ? '""' : c.old, 70) }), ' → ', h('code', { text: c.new === '' ? '""' : short(c.new) }));
    }

    function collapsible(items, show) {
        var ul = h('ul', { class: 'sfx-items' });
        items.slice(0, show).forEach(function (x) { ul.appendChild(x); });
        var rest = items.slice(show);
        var btn = h('button', { class: 'sfx-more', text: '展开其余 ' + rest.length + ' 项（多为 CMake 自己设置的 CMAKE_* 变量）' });
        btn.addEventListener('click', function () {
            rest.forEach(function (x) { ul.insertBefore(x, btn.parentNode === ul ? btn : null); });
            btn.remove();
        });
        ul.appendChild(btn);
        return ul;
    }

    function groupFiles(files) {
        // CMakeFiles/ 下面的大量内部文件按目录合并
        var out = [], groups = {};
        files.forEach(function (f) {
            var m = /^(.*?\/CMakeFiles)\/(.+)$/.exec(f.path);
            if (m && !f.blob) {
                var key = m[1] + '|' + f.op;
                if (!groups[key]) { groups[key] = { group: true, dir: m[1], op: f.op === 'del' ? 'del' : f.op, count: 0, size: 0 }; out.push(groups[key]); }
                groups[key].count++;
                groups[key].size += f.size || 0;
            } else out.push(f);
        });
        return out;
    }

    function colorOut(lines) {
        var mode = '';
        return (lines || []).map(function (l) {
            if (/^CMake (Error|Deprecation Error)/.test(l)) mode = 'err';
            else if (/^CMake (Warning|Deprecation Warning)/.test(l)) mode = 'warn';
            else if (/^(--|\S)/.test(l) && !/^ /.test(l)) mode = /error/i.test(l) && !/^--/.test(l) ? 'err' : '';
            return mode ? '<span class="' + mode + '">' + esc(l) + '</span>' : esc(l);
        }).join('\n');
    }

    function explain(step, R, k, any, txt) {
        var next = R.steps[k + 1];
        var nextLine = next && next.file === step.file ? '第 ' + next.line + ' 行' : (next ? R.display(next.file) + ':' + next.line : '');
        switch (step.cmd) {
            case 'function':
                return '只是记下函数 ' + (step.args || [''])[0] + ' 的定义，函数体现在不执行；每次调用时才在新的函数作用域里执行。';
            case 'macro':
                return '只是记下宏 ' + (step.args || [''])[0] + ' 的定义；调用时把宏体原样代入调用处执行，不新建作用域。';
            case 'if': case 'elseif': case 'while':
                return '控制流本身没有副作用。条件求值后，下一条执行的是' + (nextLine || '（没有了）') + '。';
            case 'foreach':
                return 'foreach 本身没有副作用：循环变量在每一轮开始时设好，下一条执行的是' + (nextLine || '（没有了）') + '。';
            case 'option':
                if (!any) return '缓存里已经有 ' + (step.args || [''])[0] + '，option() 什么也不做：用户（或上一次配置）给的值优先。';
                return '';
            case 'set':
                if (!any && /\bCACHE\b/.test(txt) && !/\bFORCE\b/.test(txt)) return '缓存里已经有 ' + (step.args || [''])[0] + '，不带 FORCE 的 set(... CACHE ...) 不覆盖已有的值。';
                return '';
            case 'cmake_minimum_required':
                return '除了记下最低版本，这一行还按版本范围把一批策略（policy）设成 NEW。策略不是变量，在这里看不到。';
            case 'project':
                return any ? 'project() 第一次出现时检测编译器、写下平台信息（build/CMakeFiles/<版本>/）、设置 PROJECT_* 和语言相关的 CMAKE_* 变量；结果大多进了缓存，第二次配置时直接读缓存。' : '';
            case 'add_subdirectory':
                return '';
            case 'return':
                return step.sentinel ? '' : 'return() 结束当前文件或函数。';
            case 'message':
                return any ? '' : 'message() 只往终端打印，不改任何状态。';
            case 'file':
                if ((step.args || [])[0] === 'GENERATE') return 'file(GENERATE) 只是登记一个要在生成阶段写出的文件；内容里的 $<…> 到那时才求值。结果见“生成”页。';
                return '';
            case 'add_custom_command':
                return any ? '' : '只是登记一条规则：命令写进构建文件，到构建阶段才执行。见“构建”页。';
            case 'add_custom_target':
                return '';
            case 'add_test':
                return any ? '' : '只是登记一个测试：写进 CTestTestfile.cmake（生成阶段），ctest 运行时才执行。';
            case 'install':
                return any ? '' : '只是登记一条安装规则：写进 cmake_install.cmake（生成阶段），cmake --install 时才执行。';
        }
        if (step.sentinel) return isTopFile(R, step.file) ? '顶层文件执行完，配置阶段结束。接下来 CMake 把缓存写进 CMakeCache.txt，再生成构建文件，见上面的“生成”页。' : '';
        return '';
    }

    // ---------- 右下：执行完这一步之后的完整状态

    Viewer.prototype.renderState = function () {
        var self = this, R = this.R(), step = R.steps[this.k], fx = step.fx || {};
        var st = R.after(this.k);
        var el = this.stateEl;
        el.innerHTML = '';
        var head = h('div', { class: 'sfx-state-head' });
        var tabs = h('div', { class: 'sfx-tabs' });
        var nvars = Object.keys(st.scope).length, ncache = Object.keys(st.cache).length,
            ntg = Object.keys(st.targets).length, nfiles = Object.keys(st.files).length;
        [['vars', '变量', nvars], ['cache', '缓存', ncache], ['targets', '目标', ntg], ['files', '文件', nfiles]].forEach(function (t) {
            tabs.appendChild(h('button', { class: 'sfx-tab' + (self.stateTab === t[0] ? ' on' : ''),
                onclick: function () { self.stateTab = t[0]; self.renderState(); } }, t[1], h('span', { class: 'n', text: String(t[2]) })));
        });
        head.appendChild(h('span', { class: 'cap', text: '这一步执行完以后：' }));
        head.appendChild(tabs);
        var filter = h('input', { class: 'sfx-filter', type: 'search', placeholder: '筛选…', value: this.filter,
            oninput: function () { self.filter = filter.value; self.renderStateBody(st, fx, body); } });
        head.appendChild(filter);
        if (this.stateTab === 'vars' || this.stateTab === 'cache') {
            var cb = h('input', { type: 'checkbox' });
            cb.checked = this.stateTab === 'vars' ? this.hideBuiltin : this.hideAdvanced;
            cb.addEventListener('change', function () {
                if (self.stateTab === 'vars') self.hideBuiltin = cb.checked; else self.hideAdvanced = cb.checked;
                self.renderStateBody(st, fx, body);
            });
            head.appendChild(h('label', { class: 'sfx-check' }, cb, this.stateTab === 'vars' ? '隐藏 CMAKE_* 等内置变量' : '隐藏高级项（ADVANCED）'));
        } else if (this.stateTab === 'targets') {
            var cb2 = h('input', { type: 'checkbox' });
            cb2.checked = this.hideDefaultProps;
            cb2.addEventListener('change', function () { self.hideDefaultProps = cb2.checked; self.renderStateBody(st, fx, body); });
            head.appendChild(h('label', { class: 'sfx-check' }, cb2, '只看改过的属性'));
        }
        var body = h('div', { class: 'sfx-scroll' });
        el.appendChild(head);
        el.appendChild(body);
        this.renderStateBody(st, fx, body);
    };

    Viewer.prototype.renderStateBody = function (st, fx, body) {
        var self = this, R = this.R(), q = this.filter.toLowerCase();
        body.innerHTML = '';
        function match(s) { return !q || String(s).toLowerCase().indexOf(q) >= 0; }
        if (this.stateTab === 'vars') {
            var changed = {};
            (fx.vars || []).forEach(function (v) { if (v.top) v.changes.forEach(function (c) { changed[c.name] = c.op; }); });
            var next = R.steps[Math.min(this.k + 1, R.steps.length - 1)];
            var lab = next ? scopeName(next.scopeLabel, R) : '';
            var t = h('table', { class: 'sfx-kv' });
            t.appendChild(h('tr', { class: 'grp' }, h('td', { colspan: 2, text: '当前作用域：' + lab + (this.k + 1 < R.steps.length ? '' : '（最后一步）') })));
            var names = Object.keys(st.scope).sort();
            var hidden = 0, shown = 0;
            names.forEach(function (n) {
                if (self.hideBuiltin && R.builtin(n) && !q) { hidden++; return; }
                if (!match(n) && !match(st.scope[n])) return;
                shown++;
                t.appendChild(h('tr', { class: changed[n] || '' }, h('td', { title: n, text: n }), h('td', { class: 'v', text: st.scope[n] === '' ? '""' : short(st.scope[n], 400) })));
            });
            body.appendChild(t);
            if (!shown) body.appendChild(h('div', { class: 'sfx-empty', text: q ? '没有匹配的变量。' : '还没有用户变量。' }));
            if (hidden) body.appendChild(h('div', { class: 'sfx-empty', text: '另有 ' + hidden + ' 个 CMake 内置变量被隐藏。' }));
        } else if (this.stateTab === 'cache') {
            var ch = {};
            (fx.cache || []).forEach(function (c) { ch[c.name] = c.op; });
            var t2 = h('table', { class: 'sfx-kv' });
            var hid = 0, cnt = 0;
            Object.keys(st.cache).sort().forEach(function (n) {
                var c = st.cache[n];
                if (self.hideAdvanced && (c.advanced || c.type === 'INTERNAL' || c.type === 'STATIC') && !ch[n] && !q) { hid++; return; }
                if (!match(n) && !match(c.value)) return;
                cnt++;
                t2.appendChild(h('tr', { class: ch[n] || '' },
                    h('td', { title: n }, n, h('span', { class: 'ty', text: c.type })),
                    h('td', { class: 'v' }, c.value === '' ? '""' : short(c.value, 300), c.help ? h('span', { class: 'help', text: c.help }) : null)));
            });
            body.appendChild(t2);
            if (!cnt) body.appendChild(h('div', { class: 'sfx-empty', text: '缓存是空的：还没有执行 project()，也没有 -D 参数。' }));
            if (hid) body.appendChild(h('div', { class: 'sfx-empty', text: '另有 ' + hid + ' 个高级项或内部项（ADVANCED、INTERNAL）被隐藏。' }));
        } else if (this.stateTab === 'targets') {
            var tch = {};
            (fx.targets || []).forEach(function (t) { tch[t.name] = t.op; });
            var names2 = Object.keys(st.targets).sort();
            if (!names2.length) body.appendChild(h('div', { class: 'sfx-empty', text: '还没有目标。' }));
            names2.forEach(function (n) {
                var t = st.targets[n];
                if (!match(n)) return;
                var d = h('details', { class: 'sfx-tgt ' + (tch[n] || ''), open: !!tch[n] || names2.length <= 3 });
                d.appendChild(h('summary', {}, h('b', { text: n }), h('span', { class: 'meta', text: typeZh(t.type) + (t.imported ? ' · 导入的' : '') + (t.dir ? ' · ' + (R.display(t.dir) === R.srcRoot ? '顶层' : R.display(t.dir) + '/') : '') + (t.fromTrace ? ' · 属性从命令记录还原' : '') })));
                var tb = h('table', { class: 'sfx-kv' });
                var born = t.born || {};
                Object.keys(t.props).sort(function (a, b) { return (a[0] === '@' ? 0 : 1) - (b[0] === '@' ? 0 : 1) || propName(a).localeCompare(propName(b)); }).forEach(function (p) {
                    var v = t.props[p];
                    var isDefault = p[0] !== '@' && born[p] === v;
                    if (self.hideDefaultProps && (isDefault || v === '')) return;
                    tb.appendChild(h('tr', {}, h('td', { text: propName(p) }), h('td', { class: 'v', text: v === '' ? '""' : v })));
                });
                if (!tb.children.length) tb.appendChild(h('tr', {}, h('td', { colspan: 2, class: 'v', text: '（都是创建时的默认值）' })));
                d.appendChild(tb);
                body.appendChild(d);
            });
        } else {
            var fch = {};
            (fx.files || []).forEach(function (f) { fch[f.path] = f.op; });
            body.appendChild(this.fileTree(st.files, fch, q));
        }
    };

    Viewer.prototype.fileTree = function (files, changed, q) {
        var self = this, R = this.R();
        var tree = h('div', { class: 'sfx-tree' });
        var paths = Object.keys(files).sort();
        if (q) paths = paths.filter(function (p) { return p.toLowerCase().indexOf(q) >= 0; });
        // 目录：src/、build/、prefix/ …；CMakeFiles 默认折叠
        var open = this.openDirs || (this.openDirs = {});
        var shownDirs = {};
        paths.forEach(function (p) {
            var parts = p.split('/');
            for (var i = 1; i < parts.length; i++) {
                var dir = parts.slice(0, i).join('/');
                if (shownDirs[dir]) continue;
                var parent = parts.slice(0, i - 1).join('/');
                if (i > 1 && !isOpen(parent)) return;
                shownDirs[dir] = true;
                var isO = isOpen(dir);
                var inner = paths.filter(function (x) { return x.indexOf(dir + '/') === 0; });
                var ch = inner.some(function (x) { return changed[x]; });
                tree.appendChild(h('div', { class: 'row dir' + (ch ? ' add' : ''), style: 'padding-left:' + (0.7 + (i - 1) * 1.1) + 'em',
                    onclick: (function (d, o) { return function () { open[d] = !o; self.rerenderTree(); }; })(dir, isO) },
                    h('span', { class: 'nm', text: (isO ? '▾ ' : '▸ ') + parts[i - 1] + '/' }), h('span', { class: 'sz', text: inner.length + ' 个' })));
                if (!isO) return;
            }
            var f = files[p];
            var row = h('div', { class: 'row file' + (changed[p] ? ' ' + (changed[p] === 'add' ? 'add' : 'mod') : '') + (f.blob ? ' has' : ''),
                style: 'padding-left:' + (0.7 + (parts.length - 1) * 1.1 + 1) + 'em',
                onclick: f.blob ? function () { self.showFile(p, f.blob); } : null },
                h('span', { class: 'nm', text: parts[parts.length - 1] }), h('span', { class: 'sz', text: fmtSize(f.size) }));
            tree.appendChild(row);
        });
        function isOpen(dir) {
            if (q) return true;
            if (dir in open) return open[dir];
            return !/(^|\/)(CMakeFiles|\.cmake|_deps)$/.test(dir) && dir.split('/').length <= 4;
        }
        if (!paths.length) tree.appendChild(h('div', { class: 'sfx-empty', text: '没有文件。' }));
        return tree;
    };

    Viewer.prototype.rerenderTree = function () {
        if (this.phase === 'configure') this.renderState();
        else this.renderPhase();
    };

    Viewer.prototype.showFile = function (path, blob) {
        var self = this, R = this.R();
        this.closeFile();
        var text = R.blob(blob) || '';
        this.fileView = h('div', { class: 'sfx-viewer' },
            h('div', { class: 'sfx-head' }, h('span', { class: 'sfx-title', text: R.display(path) }),
                h('span', { class: 'sfx-note', text: fmtSize(text.length) }),
                h('button', { class: 'sfx-close', 'aria-label': '关闭', onclick: function () { self.closeFile(); } }, '×')),
            h('pre', { text: text }));
        this.modal.appendChild(this.fileView);
    };
    Viewer.prototype.closeFile = function () {
        if (this.fileView) { this.fileView.remove(); this.fileView = null; }
    };

    // ------------------------------------------------------------------ 生成 / 构建 / 运行

    Viewer.prototype.renderPhase = function () {
        this.leftEl = null;
        this.ctl = null;
        this.renderHead();
        var body = h('div', { class: 'sfx-scroll' });
        var pad = h('div', { class: 'sfx-phase' });
        body.appendChild(pad);
        this.bodyEl.innerHTML = '';
        this.bodyEl.appendChild(body);
        if (this.phase === 'generate') this.renderGenerate(pad);
        else if (this.phase === 'build') this.renderBuild(pad);
        else this.renderAfter(pad);
    };

    Viewer.prototype.btChips = function (bt, ownTarget, what) {
        var self = this, R = this.R();
        bt = (bt || []).filter(function (b) { return b.line > 0; });
        if (!bt.length) return h('span', { class: 'sfx-bt', text: '（CMake 自己加的）' });
        var span = h('span', { class: 'sfx-bt' });
        span.appendChild(document.createTextNode('← '));
        bt.forEach(function (b, i) {
            if (i) span.appendChild(document.createTextNode(' ← '));
            var key = R.btKey(b.file);
            var k = R.firstStep(key, b.line);
            var label = R.display(key) + ':' + b.line + (b.cmd ? ' ' + b.cmd : '');
            if (k >= 0) span.appendChild(h('span', { class: 'sfx-link', title: '跳到配置阶段的这一步', text: label,
                onclick: function () { self.phase = 'configure'; self.k = k; self.render(); } }));
            else span.appendChild(document.createTextNode(label));
        });
        // 用法要求：这个值是经 target_link_libraries 从依赖传过来的
        var via = ownTarget && what ? this.usageVia(ownTarget, what, bt[0]) : null;
        if (via) {
            span.appendChild(document.createTextNode('  · '));
            var vt = '来自 ' + via.dep + ' 的 ' + via.prop;
            if (via.k >= 0) span.appendChild(h('span', { class: 'sfx-via sfx-link', text: vt, title: '跳到设置这个属性的那一行',
                onclick: function () { self.phase = 'configure'; self.k = via.k; self.render(); } }));
            else span.appendChild(h('span', { class: 'sfx-via', text: vt }));
        }
        return span;
    };

    // what = {kind: 'inc'|'def'|'opt'|'std'|'lib', v: 值}；找出是哪个依赖的哪个 INTERFACE_* 属性带来的
    var USAGE_PROP = { inc: ['INTERFACE_INCLUDE_DIRECTORIES', 'INTERFACE_SYSTEM_INCLUDE_DIRECTORIES'], def: ['INTERFACE_COMPILE_DEFINITIONS'],
        opt: ['INTERFACE_COMPILE_OPTIONS'], std: ['INTERFACE_COMPILE_FEATURES'], lib: ['INTERFACE_LINK_LIBRARIES', 'INTERFACE_LINK_OPTIONS'] };
    Viewer.prototype.usageVia = function (own, what, first) {
        var R = this.R();
        if (!first || first.cmd !== 'target_link_libraries') return null;
        var targets = R.afterConfigure.targets, props = USAGE_PROP[what.kind];
        if (!props) return null;
        var needle = what.kind === 'std' ? 'cxx_std_' + what.v : what.v;
        // 沿 LINK_LIBRARIES / INTERFACE_LINK_LIBRARIES 找依赖（广度优先）
        var queue = [own], seen = {}, order = [];
        while (queue.length) {
            var n = queue.shift();
            if (seen[n]) continue;
            seen[n] = 1;
            var t = targets[n];
            if (!t) continue;
            if (n !== own) order.push(n);
            var libs = (t.props['@LinkImplementations'] || '') + ';' + (t.props.INTERFACE_LINK_LIBRARIES || '');
            libs.split(';').forEach(function (x) { x = x.replace(/^\$<LINK_ONLY:(.*)>$/, '$1'); if (x) queue.push(x); });
        }
        for (var i = 0; i < order.length; i++) {
            var dep = order[i], tp = targets[dep].props;
            for (var j = 0; j < props.length; j++) {
                var v = tp[props[j]] || '';
                if (v && v.indexOf(needle) >= 0) return { dep: dep, prop: props[j], k: this.findPropStep(dep, props[j], needle) };
            }
        }
        return null;
    };
    Viewer.prototype.findPropStep = function (tgt, prop, needle) {
        var steps = this.R().steps;
        for (var k = 0; k < steps.length; k++) {
            var ts = (steps[k].fx || {}).targets || [];
            for (var i = 0; i < ts.length; i++) {
                var t = ts[i];
                if (t.name !== tgt) continue;
                var ch = t.op === 'add' ? Object.keys(t.props || {}).map(function (p) { return { name: p, new: t.props[p] }; }) : (t.changes || []);
                if (ch.some(function (c) { return c.name === prop && String(c.new || '').indexOf(needle) >= 0; })) return k;
            }
        }
        return -1;
    };

    Viewer.prototype.renderGenerate = function (pad) {
        var self = this, R = this.R(), cfg = R.cfg;
        pad.appendChild(h('p', { class: 'sfx-phase-intro', text: '配置阶段结束后，CMake 先把缓存存进 CMakeCache.txt，然后进入生成阶段：求值生成器表达式 $<…>，沿 target_link_libraries 把用法要求合并到每个目标上，最后由生成器（这里是 Ninja）写出构建文件。下面是这一阶段写出的文件和每个目标最终的编译参数；点“←”后面的位置，跳回加上这个参数的那一行。' }));
        if (cfg.tail && cfg.tail.length) {
            pad.appendChild(h('div', { class: 'sfx-h', text: '输出' }));
            pad.appendChild(h('div', { class: 'sfx-out', html: colorOut(cfg.tail) }));
        }
        var gfiles = (cfg.generate || {}).files || [];
        pad.appendChild(h('div', { class: 'sfx-h' }, '写出的文件', h('span', { class: 'sub', text: gfiles.length + ' 个，点名字看内容' })));
        var ul = h('ul', { class: 'sfx-items' });
        groupFiles(gfiles).forEach(function (f) {
            if (f.group) { ul.appendChild(item(f.op, f.op === 'add' ? '+' : '~', h('code', { text: R.display(f.dir) + '/' }), ' 下 ' + f.count + ' 个内部文件', h('span', { class: 'meta', text: ' ' + fmtSize(f.size) + (R.inSource(f.dir) ? IN_SRC : '') }))); return; }
            ul.appendChild(item(f.op === 'del' ? 'del' : f.op, f.op === 'add' ? '+' : f.op === 'del' ? '−' : '~',
                h('code', { class: f.blob ? 'sfx-link' : '', text: R.display(f.path), onclick: f.blob ? function () { self.showFile(f.path, f.blob); } : null }),
                h('span', { class: 'meta', text: '  ' + fmtSize(f.size) + '  ' + fileRole(f.path) + (R.inSource(f.path) ? IN_SRC : '') })));
        });
        pad.appendChild(ul);
        var cm = cfg.codemodel;
        if (!cm || !cm.targets || !cm.targets.length) return;
        pad.appendChild(h('div', { class: 'sfx-h' }, '每个目标最终的样子', h('span', { class: 'sub', text: '来自 CMake 的 file API（codemodel v2），已合并用法要求' })));
        var ids = {};
        cm.targets.forEach(function (t) { ids[t.id] = t.name; });
        var cards = h('div', { class: 'sfx-cards' });
        cm.targets.forEach(function (t) {
            var c = h('div', { class: 'sfx-card' });
            c.appendChild(h('h4', {}, h('code', { text: t.name }), h('span', { class: 'meta', text: typeZh(t.type) }),
                t.artifacts && t.artifacts.length ? h('span', { class: 'meta', text: '→ ' + t.artifacts.join('、') }) : null));
            if (t.sources && t.sources.length) {
                c.appendChild(h('h5', { text: '源文件' }));
                var sl = h('ul', { class: 'sfx-prov' });
                t.sources.forEach(function (s) { sl.appendChild(h('li', {}, h('code', { text: s.path }), s.generated ? h('span', { class: 'sfx-via', text: '生成的' }) : null)); });
                c.appendChild(sl);
            }
            (t.groups || []).forEach(function (g) {
                var rows = [];
                if (g.std) rows.push(h('li', {}, h('code', { text: g.lang + ' 标准 ' + g.std }), (g.stdBt || []).map(function (b) { return self.btChips(b, t.name, { kind: 'std', v: g.std }); })));
                g.includes.forEach(function (x) { rows.push(h('li', {}, h('code', { text: (x.system ? '-isystem ' : '-I') + x.v }), self.btChips(x.bt, t.name, { kind: 'inc', v: x.v }))); });
                g.defines.forEach(function (x) { rows.push(h('li', {}, h('code', { text: '-D' + x.v }), self.btChips(x.bt, t.name, { kind: 'def', v: x.v }))); });
                g.flags.forEach(function (x) { rows.push(h('li', {}, h('code', { text: x.v }), self.btChips(x.bt, t.name, { kind: 'opt', v: x.v.trim() }))); });
                if (rows.length) {
                    c.appendChild(h('h5', { text: '编译 ' + g.lang + ' 源文件时加的参数' }));
                    c.appendChild(h('ul', { class: 'sfx-prov' }, rows));
                }
            });
            var link = (t.link || []).filter(function (f) { return f.role !== 'flags' || f.v.trim(); });
            if (link.length) {
                c.appendChild(h('h5', { text: '链接' }));
                c.appendChild(h('ul', { class: 'sfx-prov' }, link.map(function (f) {
                    return h('li', {}, h('code', { text: f.v.trim() }), h('span', { class: 'meta', text: roleZh(f.role) }), self.btChips(f.bt, t.name, f.role === 'libraries' ? null : { kind: 'lib', v: f.v.trim() }));
                })));
            }
            if (t.deps && t.deps.length) {
                c.appendChild(h('h5', { text: '依赖（构建顺序）' }));
                c.appendChild(h('ul', { class: 'sfx-prov' }, t.deps.map(function (d) {
                    return h('li', {}, h('code', { text: ids[d.id] || d.id }), self.btChips(d.bt, t.name));
                })));
            }
            if (t.install && t.install.dest && t.install.dest.length) {
                c.appendChild(h('h5', { text: '安装到' }));
                c.appendChild(h('ul', { class: 'sfx-prov' }, t.install.dest.map(function (d) {
                    return h('li', {}, h('code', { text: (t.install.prefix || '') + '/' + d.path }), self.btChips(d.bt, t.name));
                })));
            }
            cards.appendChild(c);
        });
        pad.appendChild(cards);
    };

    function fileRole(p) {
        var b = p.split('/').pop();
        var roles = {
            'CMakeCache.txt': '缓存：下次配置从这里读回所有缓存变量',
            'build.ninja': 'Ninja 的构建规则：每个目标怎么编译、链接',
            'rules.ninja': 'Ninja 规则模板',
            'cmake_install.cmake': 'cmake --install 时执行的安装脚本',
            'CTestTestfile.cmake': 'ctest 读的测试清单',
            'compile_commands.json': '编译数据库：给 clangd 等工具',
            'CMakeConfigureLog.yaml': '配置日志：编译器检测、try_compile 的记录',
            'cmake.check_cache': '标记文件',
            '.ninja_log': 'Ninja 的构建日志',
            'InstallScripts.json': '安装脚本清单',
            'TargetDirectories.txt': '目标的中间目录清单'
        };
        if (roles[b]) return roles[b];
        if (/Targets(-\w+)?\.cmake$/.test(b)) return '导出的目标（给 find_package 用）';
        if (/Config(Version)?\.cmake$|-config(-version)?\.cmake$/.test(b)) return '包配置文件';
        return '';
    }

    function roleZh(r) {
        return { flags: '链接参数', libraries: '库', libraryPath: '库搜索路径', frameworkPath: '框架路径' }[r] || r;
    }

    // 构建命令里的每个参数标上来源
    Viewer.prototype.tokenMap = function (t) {
        var map = {};
        function put(tok, kind, bt, what) { if (tok && !map[tok]) map[tok] = { kind: kind, bt: bt, what: what }; }
        (t.groups || []).forEach(function (g) {
            if (g.std) (g.stdBt || []).forEach(function (b) { ['c++', 'gnu++'].forEach(function (p) { put('-std=' + p + g.std, 'opt', b, { kind: 'std', v: g.std }); }); });
            g.includes.forEach(function (x) { put('-I' + x.v, 'inc', x.bt, { kind: 'inc', v: x.v }); put(x.v, 'inc', x.bt, { kind: 'inc', v: x.v }); });
            g.defines.forEach(function (x) { put('-D' + x.v, 'def', x.bt, { kind: 'def', v: x.v }); });
            g.flags.forEach(function (x) { x.v.trim().split(/\s+/).forEach(function (tok) { put(tok, 'opt', x.bt, { kind: 'opt', v: tok }); }); });
        });
        (t.link || []).forEach(function (f) {
            f.v.trim().split(/\s+/).forEach(function (tok) { put(tok, f.role === 'libraries' ? 'lib' : 'opt', f.bt, f.role === 'libraries' ? null : { kind: 'lib', v: tok }); });
        });
        return map;
    };

    Viewer.prototype.renderBuild = function (pad) {
        var self = this, R = this.R(), b = R.run.build_result;
        var cm = R.cfg.codemodel || { targets: [] };
        pad.appendChild(h('p', { class: 'sfx-phase-intro', text: 'cmake --build 调用 Ninja，Ninja 按 build.ninja 里的规则一条条执行命令（这里用 -j 1 让顺序固定）。命令里有颜色的参数都能在 CMakeLists.txt 里找到出处，鼠标停在上面能看到，下面也逐条列出。' }));
        pad.appendChild(h('div', { class: 'sfx-legend' },
            h('span', { class: 'sfx-tok inc', text: '-I 头文件目录' }), h('span', { class: 'sfx-tok def', text: '-D 宏定义' }),
            h('span', { class: 'sfx-tok opt', text: '编译/链接选项' }), h('span', { class: 'sfx-tok lib', text: '链接的库' })));
        var ul = h('ul', { class: 'sfx-cmds' });
        (b.commands || []).forEach(function (c) {
            var info = classify(c.cmd, cm.targets);
            var li = h('li');
            li.appendChild(h('div', { class: 'what' }, h('span', { class: 'n', text: '[' + c.n + '/' + c.of + ']' }), h('b', { text: info.kind }), info.target ? h('span', {}, '目标 ', h('code', { text: info.target.name })) : null, info.what ? h('span', { class: 'sfx-note', text: info.what }) : null));
            var line = h('div', { class: 'line' });
            var map = info.target ? self.tokenMap(info.target) : {};
            var prov = [];
            var seen = {};
            splitCmd(c.cmd).forEach(function (tok, i) {
                if (i) line.appendChild(document.createTextNode(' '));
                var m = map[tok];
                if (m) {
                    var title = (m.bt || []).map(function (x) { return R.display(R.btKey(x.file)) + ':' + x.line + ' ' + (x.cmd || ''); }).join(' ← ');
                    line.appendChild(h('span', { class: 'sfx-tok ' + m.kind, title: title || 'CMake 自己加的', text: tok }));
                    if (m.bt && m.bt.some(function (b) { return b.line > 0; }) && !seen[tok]) { seen[tok] = 1; prov.push(h('li', {}, h('code', { text: tok }), self.btChips(m.bt, info.target.name, m.what))); }
                } else {
                    line.appendChild(h('span', { class: /^(:|&&|\/usr\/bin\/cmake)$/.test(tok) ? 'sfx-tok dim' : '', text: tok }));
                }
            });
            li.appendChild(line);
            if (prov.length) li.appendChild(h('div', { class: 'sfx-pad' }, h('ul', { class: 'sfx-prov' }, prov)));
            if (c.out && c.out.some(function (l) { return l; })) li.appendChild(h('div', { class: 'sfx-pad' }, h('div', { class: 'sfx-out', html: colorOut(c.out) })));
            ul.appendChild(li);
        });
        if (!(b.commands || []).length) pad.appendChild(h('div', { class: 'sfx-none', text: '没有要执行的命令：所有产物都是最新的。' }));
        pad.appendChild(ul);
        var bf = (b.files || []).filter(function (f) { return !/\.ninja_(log|deps)$/.test(f.path); });
        if (bf.length) {
            pad.appendChild(h('div', { class: 'sfx-h' }, '构建产生的文件', h('span', { class: 'sub', text: bf.length + ' 个' })));
            pad.appendChild(h('ul', { class: 'sfx-items' }, groupFiles(bf).map(function (f) {
                if (f.group) return item(f.op, '+', h('code', { text: R.display(f.dir) + '/' }), ' 下 ' + f.count + ' 个文件（目标文件 .o、依赖记录 .d 等）', h('span', { class: 'meta', text: ' ' + fmtSize(f.size) + (R.inSource(f.dir) ? IN_SRC : '') }));
                return item(f.op === 'del' ? 'del' : f.op, f.op === 'add' ? '+' : '~', h('code', { text: R.display(f.path) }), h('span', { class: 'meta', text: '  ' + fmtSize(f.size) + (R.inSource(f.path) ? IN_SRC : '') }));
            })));
        }
        if (b.exit) pad.appendChild(h('div', { class: 'sfx-out', html: colorOut(b.output.slice(-30)) }));
    };

    function splitCmd(s) {
        var out = [], cur = '', q = '';
        for (var i = 0; i < s.length; i++) {
            var c = s[i];
            if (q) { cur += c; if (c === q) q = ''; continue; }
            if (c === '"' || c === "'") { q = c; cur += c; continue; }
            if (c === ' ') { if (cur) out.push(cur); cur = ''; continue; }
            cur += c;
        }
        if (cur) out.push(cur);
        return out;
    }

    function classify(cmd, targets) {
        var toks = splitCmd(cmd);
        var oi = toks.indexOf('-o');
        var out = oi >= 0 ? toks[oi + 1] : '';
        var byName = {};
        (targets || []).forEach(function (t) { byName[t.name] = t; });
        var m = /CMakeFiles\/([^/\s]+)\.dir\//.exec(out) || /CMakeFiles\/([^/\s]+)\.dir\//.exec(cmd);
        var tgt = m ? byName[m[1]] : null;
        if (/-fdeps-format=|-fdeps-file=/.test(cmd) && toks.indexOf('-c') < 0) {
            var srcE = toks[toks.indexOf('-E') + 3] || '';
            return { kind: '扫描模块依赖', target: tgt, what: 'C++20 起，CMake 先预处理一遍源文件，找出 import/export module' };
        }
        if (/cmake_ninja_dyndep/.test(cmd)) return { kind: '整理模块依赖', target: tgt, what: 'CMake 汇总扫描结果，告诉 Ninja 编译顺序' };
        if (toks.indexOf('-c') >= 0 && tgt) {
            var src = toks[toks.indexOf('-c') + 1] || '';
            return { kind: '编译', target: tgt, what: src.replace(/^\/work\/src\//, '') + ' → ' + out };
        }
        var art = (targets || []).filter(function (t) { return (t.artifacts || []).some(function (a) { return toks.indexOf(a) >= 0; }); });
        if (/\/ar\s+q|\bar qc\b|\bar rc\b/.test(cmd)) return { kind: '打包静态库', target: art[0], what: art[0] ? (art[0].artifacts || [])[0] : '' };
        if (out && art.length) return { kind: '链接', target: art[0], what: '→ ' + out };
        if (/cmake(\s|$).*-E/.test(cmd) || /--regenerate/.test(cmd)) return { kind: 'CMake 命令', target: tgt, what: '' };
        return { kind: '自定义命令', target: tgt, what: '' };
    }

    Viewer.prototype.renderAfter = function (pad) {
        var self = this, R = this.R();
        pad.appendChild(h('p', { class: 'sfx-phase-intro', text: '构建之后依次执行的命令，以及它们改动的文件。' }));
        (R.run.after || []).forEach(function (a) {
            pad.appendChild(h('div', { class: 'sfx-h' }, a.label !== a.argv.join(' ') ? a.label : '', h('span', { class: 'sub', text: a.exit ? '退出码 ' + a.exit : '' })));
            pad.appendChild(h('div', { class: 'sfx-cmdline', text: '$ ' + a.argv.join(' ') }));
            if (a.output && a.output.length) pad.appendChild(h('div', { class: 'sfx-out', html: colorOut(a.output) }));
            var files = (a.files || []).filter(function (f) { return !/\.ninja_(log|deps)$/.test(f.path); });
            if (files.length) {
                pad.appendChild(h('ul', { class: 'sfx-items' }, groupFiles(files).map(function (f) {
                    if (f.group) return item(f.op, '+', h('code', { text: R.display(f.dir) + '/' }), ' 下 ' + f.count + ' 个文件');
                    return item(f.op === 'del' ? 'del' : f.op, f.op === 'add' ? '+' : f.op === 'del' ? '−' : '~',
                        h('code', { class: f.blob ? 'sfx-link' : '', text: R.display(f.path), onclick: f.blob ? function () { self.showFile(f.path, f.blob); } : null }),
                        h('span', { class: 'meta', text: '  ' + fmtSize(f.size) }));
                })));
            }
        });
    };

    // ------------------------------------------------------------------ 入口

    var loaded = {};
    function load(url) {
        if (!loaded[url]) {
            loaded[url] = fetch(url).then(function (r) {
                if (!r.ok) throw new Error('HTTP ' + r.status);
                return r.json();
            });
        }
        return loaded[url];
    }

    function open(url, opts) {
        opts = opts || {};
        return load(url).then(function (rec) { return new Viewer(rec, opts); }, function (err) {
            alert('读不到录制文件 ' + url + '：' + err.message);
        });
    }

    function bind(root) {
        (root || document).querySelectorAll('.sfx-launch[data-src]').forEach(function (box) {
            if (box.dataset.bound) return;
            box.dataset.bound = '1';
            var btn = box.querySelector('.sfx-open');
            if (!btn) return;
            btn.addEventListener('click', function () {
                open(box.dataset.src, { returnFocus: btn, run: +(box.dataset.run || 0), phase: box.dataset.phase || null,
                    step: box.dataset.step != null ? +box.dataset.step : null });
            });
        });
    }

    window.SideFx = { open: open, bind: bind, Viewer: Viewer, highlight: highlight };

    // 书页里的 ```cmake 代码块用同一套着色（mdBook 自带的 highlight.js 没有 CMake）
    function highlightPage(root) {
        (root || document).querySelectorAll('pre > code.language-cmake').forEach(function (code) {
            if (code.dataset.sfxHl) return;
            code.dataset.sfxHl = '1';
            code.innerHTML = code.textContent.split('\n').map(highlight).join('\n');
        });
    }

    function boot() {
        highlightPage(document);
        if (window.SIDEFX_DATA) {
            document.documentElement.classList.add('sfx-standalone');
            new Viewer(window.SIDEFX_DATA, { standalone: true });
        }
        bind(document);
    }
    if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', boot);
    else boot();
})();
