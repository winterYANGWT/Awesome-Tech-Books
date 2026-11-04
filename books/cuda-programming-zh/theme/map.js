/* 第 4 章“本章路线图”。
 *
 * 桌面（视口 ≥ 900px）且有 JS 时，.cu-map 吸在菜单栏下方。
 * 图还没开始吸顶时全部点亮；吸顶后用一条“图的下边缘再往下
 * min(120px, 视口高度的 15%)”的阅读线决定当前小节，取阅读线
 * 之上最后一个 .cu-map-at，点亮它的 data-on。
 *
 * 窄屏或没有 JS 时什么也不做：CSS 不会给 .cu-map 加吸顶和变暗，
 * 图就是章首一张普通静态图。
 *
 * 纯函数 parseOn / pickCurrent / readingLine / isPinned 不碰 DOM，
 * 供 Node 测试直接 require。
 */
(function () {
    "use strict";

    var DESKTOP = "(min-width: 900px)";

    /* ------------------------------------------------------------------ *
     * 纯函数
     * ------------------------------------------------------------------ */

    /* "a b  a" -> ["a", "b"]，去掉空格和重复项。 */
    function parseOn(value) {
        var text = String(value === null || value === undefined ? "" : value);
        var raw = text.split(/\s+/);
        var seen = {};
        var out = [];
        for (var i = 0; i < raw.length; i++) {
            var token = raw[i];
            if (!token || Object.prototype.hasOwnProperty.call(seen, token)) continue;
            seen[token] = true;
            out.push(token);
        }
        return out;
    }

    function markerTop(marker) {
        if (typeof marker === "number") return marker;
        if (marker && typeof marker.top === "number") return marker.top;
        return NaN;
    }

    /* 返回阅读线之上最后一个标记的下标；一个都没有时返回 -1。
       标记正好落在阅读线上也算“之上”，这样跳到标题时会选中它。 */
    function pickCurrent(markers, lineY) {
        if (!markers || typeof markers.length !== "number") return -1;
        if (typeof lineY !== "number" || !isFinite(lineY)) return -1;
        var current = -1;
        for (var i = 0; i < markers.length; i++) {
            var top = markerTop(markers[i]);
            if (isFinite(top) && top <= lineY) current = i;
        }
        return current;
    }

    /* 图下边缘再往下 min(120px, 视口 15%)。 */
    function readingLine(mapBottom, viewportHeight) {
        var bottom = typeof mapBottom === "number" && isFinite(mapBottom) ? mapBottom : 0;
        var vh = typeof viewportHeight === "number" && isFinite(viewportHeight) ? viewportHeight : 0;
        return bottom + Math.min(120, vh * 0.15);
    }

    /* 图的顶端还在菜单栏下沿之下（或正好到线）时算吸顶。 */
    function isPinned(mapTop, menuBottom) {
        var top = typeof mapTop === "number" && isFinite(mapTop) ? mapTop : Infinity;
        var menu = typeof menuBottom === "number" && isFinite(menuBottom) ? menuBottom : 0;
        return top <= menu + 0.5;
    }

    /* ------------------------------------------------------------------ *
     * DOM
     * ------------------------------------------------------------------ */

    var contexts = [];
    var framePending = false;

    function warn(message, err) {
        if (typeof console === "undefined" || !console.warn) return;
        if (err) console.warn("[cu-map] " + message, err);
        else console.warn("[cu-map] " + message);
    }

    function menuTop() {
        var value = parseFloat(getComputedStyle(document.documentElement)
            .getPropertyValue("--menu-bar-height"));
        return isFinite(value) && value > 0 ? value : 50;
    }

    function syncTop(ctx) {
        var top = menuTop();
        var menu = document.getElementById("mdbook-menu-bar");
        if (menu) {
            var rect = menu.getBoundingClientRect();
            top = Math.max(0, Math.min(top, rect.bottom));
        }
        top = Math.round(top);
        if (ctx.lastTop !== top) {
            ctx.figure.style.setProperty("--cu-map-top", top + "px");
            if (ctx.content) ctx.content.style.setProperty("--cu-map-top", top + "px");
            ctx.lastTop = top;
        }
        var height = Math.round(ctx.figure.offsetHeight);
        if (ctx.lastHeight !== height) {
            if (ctx.content) ctx.content.style.setProperty("--cu-map-height", height + "px");
            ctx.lastHeight = height;
        }
    }

    function headingTexts(content) {
        var list = content.querySelectorAll("h2");
        var out = [];
        for (var i = 0; i < list.length; i++) {
            out.push({ node: list[i], text: (list[i].textContent || "").trim() });
        }
        return out;
    }

    function headingBefore(headings, node) {
        var found = "";
        for (var i = 0; i < headings.length; i++) {
            if (headings[i].node === node) continue;
            /* headings[i] 在 node 前面时，node.compareDocumentPosition 会带 PRECEDING。 */
            if (node.compareDocumentPosition &&
                (node.compareDocumentPosition(headings[i].node) & 2)) {
                found = headings[i].text;
            } else if (found) {
                break;
            }
        }
        return found;
    }

    function setAllOn(ctx, on) {
        for (var i = 0; i < ctx.parts.length; i++) {
            ctx.parts[i].classList.toggle("is-on", on);
        }
    }

    function applyToken(ctx, token) {
        for (var i = 0; i < ctx.parts.length; i++) {
            if (ctx.parts[i].getAttribute("data-part") === token) {
                ctx.parts[i].classList.add("is-on");
            }
        }
    }

    function applyCurrent(ctx, index) {
        if (index === ctx.current && ctx.tokensKey === String(index)) return;
        ctx.current = index;
        ctx.tokensKey = String(index);
        if (index < 0 || !ctx.markers[index]) {
            setAllOn(ctx, true);
            if (ctx.now) ctx.now.textContent = "本章路线图";
            return;
        }
        var marker = ctx.markers[index];
        setAllOn(ctx, false);
        for (var i = 0; i < marker.on.length; i++) applyToken(ctx, marker.on[i]);
        if (ctx.now) ctx.now.textContent = marker.heading || "本章路线图";
    }

    function updateCurrent(ctx) {
        if (!ctx.live) return;
        var top = ctx.lastTop !== null ? ctx.lastTop : 0;
        var rect = ctx.figure.getBoundingClientRect();
        var pinned = isPinned(rect.top, top);
        if (pinned !== ctx.figure.classList.contains("is-pinned")) {
            ctx.figure.classList.toggle("is-pinned", pinned);
            /* 吸顶时图注换成“当前小节”那一行，高度会变，重新量一次。 */
            syncTop(ctx);
            rect = ctx.figure.getBoundingClientRect();
        }
        if (!pinned) {
            applyCurrent(ctx, -1);
            return;
        }
        var vh = window.innerHeight || document.documentElement.clientHeight || 0;
        var lineY = readingLine(rect.bottom, vh);
        var positions = [];
        for (var i = 0; i < ctx.markers.length; i++) {
            positions.push({ top: ctx.markers[i].el.getBoundingClientRect().top });
        }
        applyCurrent(ctx, pickCurrent(positions, lineY));
    }

    /* 带 hash 直接打开页面时，浏览器可能在 .is-live 生效前就滚到了标题，
       标题会被吸顶的图盖住。这里按图的高度补一次滚动。 */
    function alignHash(ctx) {
        var hash = window.location.hash;
        if (!hash || ctx.alignedHash === hash) return;
        ctx.alignedHash = hash;
        var target = null;
        try {
            target = document.getElementById(decodeURIComponent(hash.slice(1)));
        } catch (err) {
            return;
        }
        if (!target || !/^H[1-6]$/.test(target.tagName || "")) return;
        if (!(ctx.figure.compareDocumentPosition(target) & 4)) return;
        var rect = target.getBoundingClientRect();
        /* 目标在图上时，落地后图会吸顶、图注会收起；先按吸顶后的高度算。 */
        var wasPinned = ctx.figure.classList.contains("is-pinned");
        if (!wasPinned) ctx.figure.classList.add("is-pinned");
        var offset = (ctx.lastTop !== null ? ctx.lastTop : menuTop()) + ctx.figure.offsetHeight + 8;
        if (!wasPinned) ctx.figure.classList.remove("is-pinned");
        if (Math.abs(rect.top - offset) <= 6) return;
        var top = rect.top + (window.pageYOffset || document.documentElement.scrollTop || 0);
        window.scrollTo(0, Math.max(0, top - offset));
    }

    function setLive(ctx, live) {
        if (ctx.live === live) {
            if (live) {
                syncTop(ctx);
                updateCurrent(ctx);
                alignHash(ctx);
            }
            return;
        }
        ctx.live = live;
        if (live) {
            ctx.figure.classList.add("is-live", "is-sticky");
            if (ctx.content) ctx.content.classList.add("cu-map-live");
            ctx.current = -2;
            ctx.tokensKey = "";
            applyCurrent(ctx, -1);
            syncTop(ctx);
            updateCurrent(ctx);
            alignHash(ctx);
        } else {
            ctx.figure.classList.remove("is-live", "is-sticky", "is-pinned");
            if (ctx.content) ctx.content.classList.remove("cu-map-live");
            ctx.current = -2;
            ctx.tokensKey = "";
            setAllOn(ctx, false);
            if (ctx.now) ctx.now.textContent = "";
        }
    }

    function setup(figure) {
        if (!figure || figure.getAttribute("data-cu-ready") === "1") return null;
        figure.setAttribute("data-cu-ready", "1");

        var content = figure.closest ? figure.closest(".content") : null;
        if (content) content.style.overflowY = "visible";

        var ctx = {
            figure: figure,
            content: content,
            parts: figure.querySelectorAll("[data-part]"),
            markers: [],
            headings: content ? headingTexts(content) : [],
            now: null,
            current: -2,
            tokensKey: "",
            live: false,
            lastTop: null,
            lastHeight: null,
            alignedHash: "",
            mq: null
        };

        var spans = content ? content.querySelectorAll(".cu-map-at") : [];
        for (var i = 0; i < spans.length; i++) {
            var span = spans[i];
            ctx.markers.push({
                el: span,
                on: parseOn(span.getAttribute("data-on")),
                heading: headingBefore(ctx.headings, span)
            });
        }

        var caption = figure.querySelector("figcaption");
        var now = document.createElement("div");
        now.className = "cu-map-now";
        now.setAttribute("aria-live", "polite");
        if (caption && caption.parentNode) caption.parentNode.insertBefore(now, caption.nextSibling);
        else figure.appendChild(now);
        ctx.now = now;

        ctx.mq = window.matchMedia ? window.matchMedia(DESKTOP) : { matches: window.innerWidth >= 900, addEventListener: null };
        var onMode = function () {
            schedule();
        };
        if (ctx.mq.addEventListener) ctx.mq.addEventListener("change", onMode);
        else if (ctx.mq.addListener) ctx.mq.addListener(onMode);

        if (ctx.mq.matches) setLive(ctx, true);
        else setAllOn(ctx, true);
        return ctx;
    }

    function tick() {
        for (var i = 0; i < contexts.length; i++) {
            var ctx = contexts[i];
            try {
                setLive(ctx, !!ctx.mq.matches);
            } catch (err) {
                warn("更新失败", err);
            }
        }
    }

    function schedule() {
        if (framePending) return;
        framePending = true;
        var raf = typeof window.requestAnimationFrame === "function"
            ? window.requestAnimationFrame.bind(window)
            : function (fn) { return setTimeout(fn, 16); };
        raf(function () {
            framePending = false;
            tick();
        });
    }

    function init() {
        var roots = document.querySelectorAll(".cu-map[data-map]");
        if (!roots.length) return;
        Array.prototype.forEach.call(roots, function (root) {
            try {
                var ctx = setup(root);
                if (ctx) contexts.push(ctx);
            } catch (err) {
                warn("初始化失败", err);
            }
        });
        if (!contexts.length) return;

        window.addEventListener("scroll", schedule, { passive: true });
        window.addEventListener("resize", schedule);
        window.addEventListener("load", function () {
            for (var i = 0; i < contexts.length; i++) contexts[i].alignedHash = "";
            schedule();
            setTimeout(schedule, 80);
        });
        window.addEventListener("hashchange", function () {
            for (var i = 0; i < contexts.length; i++) contexts[i].alignedHash = "";
            schedule();
            setTimeout(schedule, 80);
        });
        document.addEventListener("scroll", schedule, { passive: true, capture: true });
        document.addEventListener("click", schedule, true);
        if (document.fonts && document.fonts.ready && document.fonts.ready.then) {
            document.fonts.ready.then(schedule);
        }
        schedule();
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
            parseOn: parseOn,
            pickCurrent: pickCurrent,
            readingLine: readingLine,
            isPinned: isPinned
        };
    }
})();
