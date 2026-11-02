(function () {
    'use strict';

    if (typeof path_to_root !== 'string') return;

    var WIDTH_KEY = 'atb:content-width';
    var DEFAULT_WIDTH = 750;
    var MIN_WIDTH = 600;
    var MAX_WIDTH = 1600;
    var STORE_PREFIX = 'atb:reader:';
    var RESUME_KEY = 'atb:resume';

    function clampWidth(value) {
        if (!isFinite(value)) return DEFAULT_WIDTH;
        value = Math.round(value / 10) * 10;
        return Math.min(MAX_WIDTH, Math.max(MIN_WIDTH, value));
    }

    function readWidth() {
        try {
            var raw = localStorage.getItem(WIDTH_KEY);
            if (raw === null || raw.trim() === '') return DEFAULT_WIDTH;
            return clampWidth(Number(raw));
        } catch (e) {
            return DEFAULT_WIDTH;
        }
    }

    function applyWidth(value) {
        document.documentElement.style.setProperty('--content-max-width', value + 'px');
    }

    var currentWidth = readWidth();
    applyWidth(currentWidth);

    // path_to_root is empty on every flat mdBook page, not just the index.
    var rootUrl = new URL(path_to_root || './', location.href);
    rootUrl.hash = '';
    rootUrl.search = '';
    var rootPath = rootUrl.pathname;
    var storeKey = STORE_PREFIX + rootPath;
    var readerStorageOk = true;
    var recording = false;
    var stable = false;
    var saveTimer = 0;
    var pendingResume = null;

    function emptyStore() {
        return { last: null, pages: {} };
    }

    function readStore() {
        if (!readerStorageOk) return emptyStore();
        var raw;
        try {
            raw = localStorage.getItem(storeKey);
        } catch (e) {
            readerStorageOk = false;
            return emptyStore();
        }
        if (!raw) return emptyStore();
        try {
            var data = JSON.parse(raw);
            if (!data || typeof data !== 'object' || !data.pages ||
                typeof data.pages !== 'object' || Array.isArray(data.pages)) {
                return emptyStore();
            }
            return data;
        } catch (e) {
            return emptyStore();
        }
    }

    function writeStore(data) {
        if (!readerStorageOk) return;
        try {
            localStorage.setItem(storeKey, JSON.stringify(data));
        } catch (e) {
            readerStorageOk = false;
        }
    }

    function pageKeyFromPath(pathname) {
        if (pathname === rootPath) return 'index.html';
        if (!pathname.startsWith(rootPath)) return null;
        var relative = pathname.slice(rootPath.length).replace(/^\/+/, '');
        if (relative === '') return 'index.html';
        return relative.endsWith('/') ? relative + 'index.html' : relative;
    }

    function pageKeyFromHref(href) {
        try {
            var url = new URL(href, location.href);
            if (url.origin !== rootUrl.origin) return null;
            return pageKeyFromPath(url.pathname);
        } catch (e) {
            return null;
        }
    }

    function currentPathKey() {
        return pageKeyFromPath(location.pathname) || 'index.html';
    }

    function isBookHomePath() {
        var path = location.pathname;
        if (path === rootPath) return true;
        if (!path.startsWith(rootPath)) return false;
        var relative = path.slice(rootPath.length);
        return relative === '' || relative === 'index.html';
    }

    function referrerInsideBook() {
        if (!document.referrer) return false;
        try {
            var referrer = new URL(document.referrer);
            if (referrer.origin !== rootUrl.origin) return false;
            return referrer.pathname.startsWith(rootPath);
        } catch (e) {
            return false;
        }
    }

    function navigationType() {
        try {
            var entry = performance.getEntriesByType('navigation')[0];
            return entry ? entry.type : '';
        } catch (e) {
            return '';
        }
    }

    function readResumeFlag() {
        var value = null;
        try {
            value = sessionStorage.getItem(RESUME_KEY);
        } catch (e) {
            return false;
        }
        if (value !== null) {
            try {
                sessionStorage.removeItem(RESUME_KEY);
            } catch (e) { /* ignore */ }
        }
        return value === '1';
    }

    function prepareResume() {
        if (!isBookHomePath() || location.hash) return false;
        try {
            var params = new URLSearchParams(location.search);
            if (params.has('search') || params.has('highlight')) return false;
        } catch (e) {
            return false;
        }
        if (referrerInsideBook()) return false;
        if (navigationType() !== 'navigate') return false;

        var data = readStore();
        var last = data.last;
        if (typeof last !== 'string' || last === '') return false;
        if (last === currentPathKey()) return false;

        pendingResume = last;
        document.documentElement.style.visibility = 'hidden';
        return true;
    }

    function findPageLink(pageKey) {
        var sidebar = document.getElementById('mdbook-sidebar');
        if (!sidebar) return null;
        var links = sidebar.querySelectorAll('a[href]');
        var fallback = null;

        for (var i = 0; i < links.length; i++) {
            var link = links[i];
            var href = link.getAttribute('href');
            if (!href || pageKeyFromHref(link.href) !== pageKey) continue;
            if (href.indexOf('#') !== -1) {
                if (!fallback) fallback = link;
                continue;
            }
            return link;
        }
        return fallback;
    }

    function resumePendingPage() {
        if (!pendingResume) return false;
        var link = findPageLink(pendingResume);
        if (!link) {
            pendingResume = null;
            document.documentElement.style.removeProperty('visibility');
            return false;
        }

        try {
            sessionStorage.setItem(RESUME_KEY, '1');
        } catch (e) { /* navigation can still proceed */ }
        try {
            location.replace(link.href);
            return true;
        } catch (e) {
            try {
                sessionStorage.removeItem(RESUME_KEY);
            } catch (ignore) { /* ignore */ }
            pendingResume = null;
            document.documentElement.style.removeProperty('visibility');
            return false;
        }
    }

    var resumeFlag = readResumeFlag();

    prepareResume();

    document.addEventListener('DOMContentLoaded', init);

    function init() {
        if (resumePendingPage()) return;
        var menu = document.getElementById('mdbook-menu-bar');
        var main = document.querySelector('#mdbook-content > main');
        if (menu) initSettings(menu, main);
        if (main) initReading(main);
    }

    function menuHeight() {
        var value = parseFloat(getComputedStyle(document.documentElement)
            .getPropertyValue('--menu-bar-height'));
        return isFinite(value) ? value : 50;
    }

    function scrollingElement() {
        return document.scrollingElement || document.documentElement;
    }

    function captureAnchor(main) {
        var children = main.children;
        if (!children.length) return null;

        var scroller = scrollingElement();
        var topLine = scroller.scrollTop + menuHeight();
        var i;
        var rect;
        var top;

        for (i = 0; i < children.length; i++) {
            rect = children[i].getBoundingClientRect();
            top = rect.top + scroller.scrollTop;
            if (topLine < top) return { i: i, f: 0 };
            if (topLine < top + rect.height) {
                return {
                    i: i,
                    f: Math.min(1, Math.max(0, (topLine - top) / rect.height))
                };
            }
        }
        return { i: children.length - 1, f: 1 };
    }

    function scrollToAnchor(main, index, fraction) {
        var count = main.children.length;
        var i = Number(index);
        if (!Number.isInteger(i) || i < 0 || i >= count) return false;

        var element = main.children[i];
        var rect = element.getBoundingClientRect();
        var scroller = scrollingElement();
        var top = rect.top + scroller.scrollTop;
        var f = Number(fraction);
        if (!isFinite(f)) f = 0;
        f = Math.min(1, Math.max(0, f));
        var maxScroll = Math.max(0, scroller.scrollHeight - scroller.clientHeight);
        var target = Math.max(0, Math.min(maxScroll,
            top + rect.height * f - menuHeight()));
        window.scrollTo(0, target);
        return true;
    }

    function restoreAnchor(main, anchor) {
        if (anchor) scrollToAnchor(main, anchor.i, anchor.f);
    }

    function recordFor(main) {
        var anchor = captureAnchor(main) || { i: 0, f: 0 };
        var scroller = scrollingElement();
        var maxScroll = scroller.scrollHeight - scroller.clientHeight;
        var ratio = maxScroll > 0 ? scroller.scrollTop / maxScroll : 0;
        return {
            i: anchor.i,
            f: anchor.f,
            n: main.children.length,
            r: Math.min(1, Math.max(0, ratio)),
            t: Date.now()
        };
    }

    function savePage(main, pageKey) {
        if (!recording || stable || !main || !pageKey) return;
        var data = readStore();
        data.pages[pageKey] = recordFor(main);
        data.last = pageKey;
        writeStore(data);
    }

    function scheduleSave(main, pageKey) {
        if (!recording || stable) return;
        clearTimeout(saveTimer);
        saveTimer = setTimeout(function () {
            saveTimer = 0;
            savePage(main, pageKey);
        }, 300);
    }

    function flushSave(main, pageKey) {
        clearTimeout(saveTimer);
        saveTimer = 0;
        savePage(main, pageKey);
    }

    function listenForSaves(main, pageKey) {
        window.addEventListener('scroll', function () {
            scheduleSave(main, pageKey);
        }, { passive: true });
        window.addEventListener('pagehide', function () {
            flushSave(main, pageKey);
        });
        document.addEventListener('visibilitychange', function () {
            if (document.visibilityState === 'hidden') {
                flushSave(main, pageKey);
            }
        });
    }

    function scrollToRecord(main, record) {
        if (Number(record.n) === main.children.length &&
            scrollToAnchor(main, record.i, record.f)) {
            return;
        }

        var scroller = scrollingElement();
        var maxScroll = Math.max(0, scroller.scrollHeight - scroller.clientHeight);
        var ratio = Number(record.r);
        if (!isFinite(ratio)) ratio = 0;
        var target = Math.min(1, Math.max(0, ratio)) * maxScroll;
        window.scrollTo(0, Math.max(0, Math.min(maxScroll, target)));
    }

    // MathJax 2.7 loads async from a CDN, so it can start typesetting long after the page looks settled.
    function whenMathDone(callback) {
        var script = document.querySelector('script[src*="MathJax.js"]');
        if (!script) {
            callback();
            return;
        }

        var done = false;
        function once() {
            if (done) return;
            done = true;
            callback();
        }

        function hook() {
            if (window.MathJax && window.MathJax.Hub) {
                try {
                    window.MathJax.Hub.Register.StartupHook('End', once);
                    return;
                } catch (e) { /* fall through */ }
            }
            once();
        }

        if (window.MathJax && window.MathJax.Hub) {
            hook();
            return;
        }
        script.addEventListener('load', hook);
        script.addEventListener('error', once);
    }

    function beginStable(main, record, pageKey) {
        stable = true;
        var settleTimer = 0;
        var maxTimer = 0;
        var observer = null;
        var mathDone = false;

        function finish() {
            if (!stable) return;
            stable = false;
            clearTimeout(settleTimer);
            clearTimeout(maxTimer);
            window.removeEventListener('load', onLoad);
            if (observer) observer.disconnect();
            removeInteractionListeners();
            recording = true;
            savePage(main, pageKey);
        }

        function scheduleSettle() {
            if (!stable || !mathDone) return;
            clearTimeout(settleTimer);
            settleTimer = setTimeout(finish, 800);
        }

        function restore() {
            if (stable) scrollToRecord(main, record);
        }

        function onLoad() {
            restore();
        }

        function onInteraction() {
            finish();
        }

        function onMathDone() {
            if (!stable) return;
            mathDone = true;
            restore();
            scheduleSettle();
        }

        var interactionTypes = ['wheel', 'touchstart', 'keydown', 'pointerdown'];
        function removeInteractionListeners() {
            interactionTypes.forEach(function (type) {
                window.removeEventListener(type, onInteraction, true);
            });
        }

        window.addEventListener('load', onLoad);
        interactionTypes.forEach(function (type) {
            window.addEventListener(type, onInteraction, {
                capture: true,
                passive: true
            });
        });

        if (document.fonts && document.fonts.ready &&
            typeof document.fonts.ready.then === 'function') {
            document.fonts.ready.then(restore, function () { /* ignore */ });
        }

        if (typeof ResizeObserver === 'function') {
            try {
                observer = new ResizeObserver(function () {
                    restore();
                    scheduleSettle();
                });
                observer.observe(main);
            } catch (e) {
                observer = null;
            }
        }

        restore();
        // Typesetting the longest formula chapters takes over 10s even on a fast network.
        var hasMath = !!document.querySelector('script[src*="MathJax.js"]');
        maxTimer = setTimeout(finish, hasMath ? 30000 : 8000);
        whenMathDone(onMathDone);
    }

    function markVisited() {
        var sidebar = document.getElementById('mdbook-sidebar');
        if (!sidebar) return;
        var data = readStore();
        var pages = data.pages;
        var links = sidebar.querySelectorAll('a[href]');

        Array.prototype.forEach.call(links, function (link) {
            var href = link.getAttribute('href');
            if (!href || href.indexOf('#') !== -1) return;
            var key = pageKeyFromHref(link.href);
            if (key && Object.prototype.hasOwnProperty.call(pages, key)) {
                link.classList.add('atb-visited');
            }
        });
    }

    function initReading(main) {
        var pageKey = (function () {
            var active = document.querySelector('#mdbook-sidebar a.active');
            return (active && pageKeyFromHref(active.href)) ||
                pageKeyFromHref(location.href) || 'index.html';
        })();

        listenForSaves(main, pageKey);

        var data = readStore();
        var record = data.pages[pageKey];
        var hasRecord = !!record && typeof record === 'object' && !Array.isArray(record);
        var blocked = !!location.hash;
        if (!blocked) {
            try {
                var params = new URLSearchParams(location.search);
                blocked = params.has('search') || params.has('highlight');
            } catch (e) {
                blocked = false;
            }
        }

        if (!hasRecord || blocked) {
            recording = true;
            savePage(main, pageKey);
            markVisited();
            return;
        }

        try {
            history.scrollRestoration = 'manual';
        } catch (e) { /* ignore */ }

        var finished = Number(record.r) >= 0.98;
        var navType = navigationType();
        var keepExact = resumeFlag ||
            navType === 'reload' ||
            navType === 'back_forward';

        if (finished && !keepExact) {
            window.scrollTo(0, 0);
            recording = true;
            savePage(main, pageKey);
        } else {
            beginStable(main, record, pageKey);
        }
        markVisited();
    }

    function initSettings(menu, main) {
        var rightButtons = menu.querySelector('.right-buttons');
        if (!rightButtons) return;

        var button = document.createElement('button');
        button.type = 'button';
        button.className = 'icon-button atb-reader-toggle';
        button.title = '阅读设置';
        button.setAttribute('aria-label', '阅读设置');
        button.setAttribute('aria-expanded', 'false');
        button.setAttribute('aria-controls', 'atb-reader-panel');
        button.innerHTML =
            '<span class="fa-svg">' +
            '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512" ' +
            'fill="currentColor" aria-hidden="true" focusable="false">' +
            '<path d="M495.9 166.6c3.2 8.7.5 18.4-6.4 24.6l-43.3 39.4c1.1 8.3 1.7 16.8 1.7 25.4s-.6 17.1-1.7 25.4l43.3 39.4c6.9 6.2 9.6 15.9 6.4 24.6-4.4 11.9-9.7 23.3-15.8 34.3l-4.7 8.1c-6.6 11-14 21.4-22.1 31.2-5.9 7.2-15.7 9.6-24.5 6.8l-55.7-17.7c-13.4 10.3-28.2 18.9-44 25.4l-12.5 57.1c-2 9.1-9 16.3-18.2 17.8-13.8 2.3-28 3.5-42.5 3.5s-28.7-1.2-42.5-3.5c-9.2-1.5-16.2-8.7-18.2-17.8l-12.5-57.1c-15.8-6.5-30.6-15.1-44-25.4l-55.7 17.7c-8.8 2.8-18.6.3-24.5-6.8-8.1-9.8-15.5-20.2-22.1-31.2l-4.7-8.1c-6.1-11-11.4-22.4-15.8-34.3-3.2-8.7-.5-18.4 6.4-24.6l43.3-39.4C64.6 273.1 64 264.6 64 256s.6-17.1 1.7-25.4L22.4 191.2c-6.9-6.2-9.6-15.9-6.4-24.6 4.4-11.9 9.7-23.3 15.8-34.3l4.7-8.1c6.6-11 14-21.4 22.1-31.2 5.9-7.2 15.7-9.6 24.5-6.8l55.7 17.7c13.4-10.3 28.2-18.9 44-25.4l12.5-57.1c2-9.1 9-16.3 18.2-17.8C227.3 1.2 241.5 0 256 0s28.7 1.2 42.5 3.5c9.2 1.5 16.2 8.7 18.2 17.8l12.5 57.1c15.8 6.5 30.6 15.1 44 25.4l55.7-17.7c8.8-2.8 18.6-.3 24.5 6.8 8.1 9.8 15.5 20.2 22.1 31.2l4.7 8.1c6.1 11 11.4 22.4 15.8 34.3zM256 336a80 80 0 1 0 0-160 80 80 0 1 0 0 160z"/>' +
            '</svg></span>';
        rightButtons.insertBefore(button, rightButtons.firstChild);

        var panel = document.createElement('div');
        panel.id = 'atb-reader-panel';
        panel.className = 'atb-reader-panel';
        panel.hidden = true;
        panel.setAttribute('role', 'dialog');
        panel.setAttribute('aria-label', '阅读设置');
        panel.innerHTML =
            '<div class="atb-reader-row">' +
            '<label for="atb-reader-width">内容宽度</label>' +
            '<output id="atb-reader-width-value" for="atb-reader-width">' +
            currentWidth + ' px</output>' +
            '</div>' +
            '<input id="atb-reader-width" class="atb-reader-range" ' +
            'type="range" min="600" max="1600" step="10" ' +
            'aria-label="内容宽度">' +
            '<div class="atb-reader-actions">' +
            '<button type="button" class="atb-reader-reset">恢复默认</button>' +
            '</div>';
        document.body.appendChild(panel);

        var range = panel.querySelector('#atb-reader-width');
        var valueOut = panel.querySelector('#atb-reader-width-value');
        var reset = panel.querySelector('.atb-reader-reset');
        range.value = String(currentWidth);

        function syncUi() {
            range.value = String(currentWidth);
            valueOut.textContent = currentWidth + ' px';
        }

        function setWidth(value, persist) {
            var next = clampWidth(value);
            var anchor = main ? captureAnchor(main) : null;
            currentWidth = next;
            applyWidth(next);
            if (main && anchor) restoreAnchor(main, anchor);
            syncUi();

            if (persist === 'remove') {
                try {
                    localStorage.removeItem(WIDTH_KEY);
                } catch (e) { /* ignore */ }
            } else if (persist) {
                try {
                    localStorage.setItem(WIDTH_KEY, String(next));
                } catch (e) { /* ignore */ }
            }
        }

        function openPanel() {
            panel.hidden = false;
            button.setAttribute('aria-expanded', 'true');
            try {
                range.focus({ preventScroll: true });
            } catch (e) {
                range.focus();
            }
        }

        function closePanel() {
            if (panel.hidden) return;
            panel.hidden = true;
            button.setAttribute('aria-expanded', 'false');
            try {
                button.focus({ preventScroll: true });
            } catch (e) {
                button.focus();
            }
        }

        button.addEventListener('click', function () {
            if (panel.hidden) {
                openPanel();
            } else {
                closePanel();
            }
        });

        range.addEventListener('input', function () {
            setWidth(Number(range.value), true);
        });

        reset.addEventListener('click', function () {
            setWidth(DEFAULT_WIDTH, 'remove');
        });

        panel.addEventListener('keydown', function (event) {
            event.stopPropagation();
            if (event.key === 'Escape') {
                event.preventDefault();
                closePanel();
            }
        });

        document.addEventListener('click', function (event) {
            if (panel.hidden) return;
            if (panel.contains(event.target) || button.contains(event.target)) return;
            closePanel();
        });

        document.addEventListener('keydown', function (event) {
            if (!panel.hidden && event.key === 'Escape') {
                event.preventDefault();
                closePanel();
            }
        });
    }
})();
