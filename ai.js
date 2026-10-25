/**
 * ╔═══════════════════════════════════════════════════════╗
 * ║         PlantBot AI — Smart Plant Manager             ║
 * ║  Standalone AI chatbot module using Groq API          ║
 * ║  Include this file AFTER index.html body content      ║
 * ╚═══════════════════════════════════════════════════════╝
 *
 * Dependencies: None (vanilla JS, no frameworks)
 * API: Groq (https://api.groq.com)
 * Usage: <script src="ai.js"></script>
 */

(function () {
  'use strict';

  /* ────────────────────────────────────────────────────────
     CONFIG — API key loaded dynamically from localStorage or window
     Set locally via: localStorage.setItem('GROQ_API_KEY', 'gsk_...');
  ──────────────────────────────────────────────────────── */
  let GROQ_API_KEY = window.GROQ_API_KEY || localStorage.getItem('GROQ_API_KEY') || '';
  const GROQ_ENDPOINT = 'https://api.groq.com/openai/v1/chat/completions';

  const AVAILABLE_MODELS = [
    { id: 'llama3-70b-8192',              label: 'Llama3 70B' },
    { id: 'llama3-8b-8192',               label: 'Llama3 8B' },
    { id: 'mixtral-8x7b-32768',           label: 'Mixtral 8x7B' },
    { id: 'gemma2-9b-it',                 label: 'Gemma2 9B' },
    { id: 'llama-3.1-8b-instant',         label: 'Llama 3.1 8B' },
    { id: 'llama-3.1-70b-versatile',      label: 'Llama 3.1 70B' },
    { id: 'deepseek-r1-distill-llama-70b',label: 'DeepSeek R1' },
  ];

  const SYSTEM_PROMPT = `You are PlantBot 🌿, an expert plant care AI assistant built into Smart Plant Manager.
You specialize in: plant watering schedules, soil types, plant diseases, fertilizers, sunlight needs, indoor/outdoor plants, and gardening tips.
Keep answers friendly, helpful, and concise. Use emojis occasionally. Format with bullet points when listing multiple items.
If asked about a specific plant, give tailored personalised advice.`;

  const QUICK_SUGGESTIONS = [
    { emoji: '💧', label: 'Watering tips',   text: 'How often should I water Aloe Vera?' },
    { emoji: '🌱', label: 'Beginner plants', text: 'Best indoor plants for beginners?' },
    { emoji: '🟡', label: 'Yellow leaves',   text: 'Why are my plant leaves turning yellow?' },
    { emoji: '🪴', label: 'Soil tips',        text: 'Best soil type for succulents?' },
    { emoji: '💚', label: 'Revive plant',    text: 'How to revive a dead or dying plant?' },
  ];

  /* ────────────────────────────────────────────────────────
     STATE
  ──────────────────────────────────────────────────────── */
  let chatOpen   = false;
  let isTyping   = false;
  let chatHistory = [];   // { role, content }[]  — kept for multi-turn context

  /* ────────────────────────────────────────────────────────
     INJECT CSS
  ──────────────────────────────────────────────────────── */
  function injectStyles() {
    const style = document.createElement('style');
    style.id = 'plantbot-styles';
    style.textContent = `
      /* ── FAB ── */
      #plantbot-fab {
        position: fixed;
        bottom: 28px;
        right: 28px;
        z-index: 2147483647;       /* max z-index, beats everything */
        width: 62px;
        height: 62px;
        border-radius: 50%;
        background: linear-gradient(135deg, #1e5631 0%, #3da85e 100%);
        color: #fff;
        border: none;
        font-size: 26px;
        cursor: pointer;
        box-shadow: 0 6px 32px rgba(30,86,49,.55);
        display: flex;
        align-items: center;
        justify-content: center;
        transition: transform .25s, box-shadow .25s;
        animation: pb-pulse 2.8s infinite;
        pointer-events: auto;       /* ensure clicks register */
        isolation: isolate;
      }
      @keyframes pb-pulse {
        0%,100% { box-shadow: 0 6px 28px rgba(30,86,49,.5); }
        50%      { box-shadow: 0 6px 44px rgba(30,86,49,.85), 0 0 0 10px rgba(30,86,49,.1); }
      }
      #plantbot-fab:hover { transform: scale(1.12); }
      #plantbot-fab.open  { transform: scale(1.08) rotate(15deg); animation: none; }

      #plantbot-badge {
        position: absolute;
        top: -4px; right: -4px;
        width: 20px; height: 20px;
        border-radius: 50%;
        background: #ef4444;
        color: #fff;
        font-size: 11px;
        font-weight: 800;
        display: none;
        align-items: center;
        justify-content: center;
        font-family: 'Nunito', sans-serif;
        border: 2px solid #fff;
        pointer-events: none;
      }
      #plantbot-badge.show { display: flex; }

      /* ── WINDOW ── */
      #plantbot-window {
        position: fixed;
        bottom: 104px;
        right: 28px;
        z-index: 2147483646;
        width: 400px;
        height: 580px;
        background: var(--card-bg, #fff);
        border-radius: 26px;
        box-shadow: 0 28px 90px rgba(0,0,0,.28), 0 0 0 1.5px rgba(45,138,78,.18);
        display: none;
        flex-direction: column;
        overflow: hidden;
        border: 1.5px solid var(--border, #d1fae5);
        font-family: 'Nunito', sans-serif;
        pointer-events: auto;
      }
      #plantbot-window.open {
        display: flex;
        animation: pb-slideup .28s cubic-bezier(.22,.68,0,1.4);
      }
      @keyframes pb-slideup {
        from { opacity:0; transform: translateY(28px) scale(.94); }
        to   { opacity:1; transform: translateY(0)    scale(1);   }
      }

      /* ── HEADER ── */
      .pb-header {
        background: linear-gradient(135deg, #1e5631 0%, #2d8a4e 100%);
        color: #fff;
        padding: 14px 16px;
        display: flex;
        align-items: center;
        gap: 10px;
        flex-shrink: 0;
      }
      .pb-avatar {
        width: 38px; height: 38px;
        border-radius: 50%;
        background: rgba(255,255,255,.18);
        display: flex; align-items: center; justify-content: center;
        font-size: 20px; flex-shrink: 0;
      }
      .pb-header-info { flex: 1; }
      .pb-header-info .pb-name  { font-weight: 800; font-size: 15px; }
      .pb-header-info .pb-status {
        font-size: 11px; opacity: .8;
        display: flex; align-items: center; gap: 4px; margin-top: 1px;
      }
      .pb-dot {
        width: 7px; height: 7px; border-radius: 50%;
        background: #6ee7a0;
        display: inline-block;
        animation: pb-blink 2s infinite;
      }
      @keyframes pb-blink { 0%,100%{opacity:1} 50%{opacity:.4} }

      .pb-model-wrap { display: flex; flex-direction: column; align-items: flex-end; gap: 3px; }
      .pb-model-label { font-size: 10px; opacity: .65; }
      .pb-model-sel {
        background: rgba(255,255,255,.16);
        border: 1px solid rgba(255,255,255,.28);
        color: #fff;
        padding: 4px 8px;
        border-radius: 8px;
        font-size: 11px;
        font-family: 'Nunito', sans-serif;
        cursor: pointer;
        max-width: 150px;
      }
      .pb-model-sel option { background: #1e5631; color: #fff; }

      .pb-btn-icon {
        background: rgba(255,255,255,.14);
        border: none; color: #fff;
        width: 30px; height: 30px;
        border-radius: 50%;
        font-size: 15px;
        cursor: pointer;
        display: flex; align-items: center; justify-content: center;
        transition: background .2s;
        flex-shrink: 0;
      }
      .pb-btn-icon:hover { background: rgba(255,255,255,.28); }

      /* ── SUGGESTIONS ── */
      .pb-suggestions {
        display: flex; flex-wrap: wrap; gap: 6px;
        padding: 10px 14px 8px;
        border-bottom: 1px solid var(--border, #d1fae5);
        background: var(--card-bg, #fff);
        flex-shrink: 0;
      }
      .pb-suggest {
        padding: 5px 12px;
        border-radius: 20px;
        border: 1.5px solid var(--green, #2d8a4e);
        background: var(--card-bg, #fff);
        color: var(--green-dark, #1e5631);
        font-size: 11px; font-weight: 700;
        cursor: pointer;
        transition: all .15s;
        font-family: 'Nunito', sans-serif;
        white-space: nowrap;
      }
      .pb-suggest:hover { background: var(--green, #2d8a4e); color: #fff; }

      /* ── MESSAGES ── */
      .pb-messages {
        flex: 1;
        overflow-y: auto;
        padding: 14px 14px 6px;
        display: flex; flex-direction: column; gap: 10px;
        scroll-behavior: smooth;
      }
      .pb-messages::-webkit-scrollbar { width: 4px; }
      .pb-messages::-webkit-scrollbar-thumb { background: var(--green-light, #4caf70); border-radius: 2px; }

      .pb-msg-wrap { display: flex; gap: 8px; align-items: flex-end; }
      .pb-msg-wrap.user { flex-direction: row-reverse; }

      .pb-msg-avatar {
        width: 28px; height: 28px; border-radius: 50%;
        background: var(--green-pale, #e8f5ec);
        display: flex; align-items: center; justify-content: center;
        font-size: 14px; flex-shrink: 0;
      }
      .pb-msg-wrap.user .pb-msg-avatar { background: var(--green, #2d8a4e); }

      .pb-msg {
        max-width: 80%;
        padding: 10px 14px;
        border-radius: 18px;
        font-size: 13px; line-height: 1.6;
        word-wrap: break-word;
      }
      .pb-msg.bot {
        background: var(--green-pale, #e8f5ec);
        color: var(--text, #1a2e22);
        border-bottom-left-radius: 4px;
      }
      .pb-msg.user {
        background: linear-gradient(135deg, #2d8a4e, #1e5631);
        color: #fff;
        border-bottom-right-radius: 4px;
      }
      .pb-msg-time {
        font-size: 10px; opacity: .5;
        margin-top: 3px; text-align: right;
        font-family: 'Nunito', sans-serif;
      }

      /* ── TYPING ── */
      .pb-typing-dots { display: flex; gap: 4px; align-items: center; padding: 4px 0; }
      .pb-typing-dots span {
        width: 7px; height: 7px; border-radius: 50%;
        background: var(--green, #2d8a4e);
        display: inline-block;
        animation: pb-bounce 1.2s infinite;
      }
      .pb-typing-dots span:nth-child(2) { animation-delay: .2s; }
      .pb-typing-dots span:nth-child(3) { animation-delay: .4s; }
      @keyframes pb-bounce {
        0%,60%,100% { transform: translateY(0); opacity:.4; }
        30%          { transform: translateY(-6px); opacity:1; }
      }

      /* ── INPUT AREA ── */
      .pb-input-wrap {
        display: flex; gap: 8px; padding: 10px 14px 14px;
        border-top: 1px solid var(--border, #d1fae5);
        background: var(--card-bg, #fff);
        flex-shrink: 0; align-items: flex-end;
      }
      .pb-input {
        flex: 1;
        padding: 10px 14px;
        border-radius: 20px;
        border: 2px solid var(--border, #d1fae5);
        background: var(--input-bg, #fff);
        color: var(--text, #1a2e22);
        font-family: 'Nunito', sans-serif;
        font-size: 13px;
        outline: none; resize: none;
        max-height: 80px; min-height: 40px;
        transition: border .2s;
        line-height: 1.4;
      }
      .pb-input:focus { border-color: var(--green, #2d8a4e); }
      .pb-input::placeholder { color: var(--text-light, #5a7a63); opacity: .7; }

      .pb-send {
        width: 42px; height: 42px;
        border-radius: 50%;
        background: linear-gradient(135deg, #2d8a4e, #1e5631);
        color: #fff; border: none; font-size: 16px;
        cursor: pointer;
        display: flex; align-items: center; justify-content: center;
        transition: transform .2s, opacity .2s;
        flex-shrink: 0;
      }
      .pb-send:hover  { transform: scale(1.1); }
      .pb-send:disabled { opacity: .45; cursor: not-allowed; transform: none; }

      /* ── DARK MODE SUPPORT ── */
      body.dark #plantbot-window {
        background: var(--card-bg, #182c20);
        border-color: var(--border, #2d5a3d);
      }
      body.dark .pb-msg.bot  { background: var(--green-pale, #1a2e22); color: var(--text, #e0f0e8); }
      body.dark .pb-suggestions { background: var(--card-bg, #182c20); }
      body.dark .pb-input-wrap  { background: var(--card-bg, #182c20); }
      body.dark .pb-suggest     { background: var(--card-bg, #182c20); color: var(--green-light, #6ee7a0); border-color: var(--green, #3da85e); }
      body.dark .pb-suggest:hover { background: var(--green, #3da85e); color: #fff; }
    `;
    document.head.appendChild(style);
  }

  /* ────────────────────────────────────────────────────────
     BUILD HTML
  ──────────────────────────────────────────────────────── */
  function buildHTML() {
    /* FAB button */
    const fab = document.createElement('button');
    fab.id = 'plantbot-fab';
    fab.title = 'Chat with PlantBot AI';
    fab.setAttribute('aria-label', 'Open PlantBot AI chat');
    fab.innerHTML = `🤖<div class="fab-badge" id="plantbot-badge">1</div>`;
    fab.addEventListener('click', toggleChat);
    document.body.appendChild(fab);

    /* Chat window */
    const win = document.createElement('div');
    win.id = 'plantbot-window';
    win.setAttribute('aria-live', 'polite');
    win.innerHTML = buildWindowHTML();
    document.body.appendChild(win);

    /* Wire up events after DOM insertion */
    wireEvents();
  }

  function buildWindowHTML() {
    const modelOptions = AVAILABLE_MODELS.map(m =>
      `<option value="${m.id}">${m.label}</option>`
    ).join('');

    const suggestions = QUICK_SUGGESTIONS.map(s =>
      `<button class="pb-suggest" data-text="${escapeAttr(s.text)}">${s.emoji} ${s.label}</button>`
    ).join('');

    return `
      <div class="pb-header">
        <div class="pb-avatar">🌿</div>
        <div class="pb-header-info">
          <div class="pb-name">PlantBot AI</div>
          <div class="pb-status"><span class="pb-dot"></span> Online &bull; Plant Care Expert</div>
        </div>
        <div class="pb-model-wrap">
          <div class="pb-model-label">Model</div>
          <select class="pb-model-sel" id="pb-model-sel">${modelOptions}</select>
        </div>
        <button class="pb-btn-icon" id="pb-clear-btn" title="Clear chat">🗑</button>
        <button class="pb-btn-icon" id="pb-close-btn" title="Close">✕</button>
      </div>

      <div class="pb-suggestions" id="pb-suggestions">${suggestions}</div>

      <div class="pb-messages" id="pb-messages">
        ${makeWelcomeMsg()}
      </div>

      <div class="pb-input-wrap">
        <textarea
          class="pb-input"
          id="pb-input"
          placeholder="Ask about plant care…"
          rows="1"
          aria-label="Chat input"
        ></textarea>
        <button class="pb-send" id="pb-send-btn" title="Send" aria-label="Send message">➤</button>
      </div>
    `;
  }

  function makeWelcomeMsg() {
    return `
      <div class="pb-msg-wrap">
        <div class="pb-msg-avatar">🌿</div>
        <div>
          <div class="pb-msg bot">
            Hi! I'm <strong>PlantBot</strong> 🌱 — your AI plant care assistant!<br><br>
            Ask me anything about watering schedules, soil types, plant diseases, or gardening tips. Let's keep your garden thriving! 🌿
          </div>
          <div class="pb-msg-time">${nowTime()}</div>
        </div>
      </div>
    `;
  }

  function wireEvents() {
    document.getElementById('pb-close-btn').addEventListener('click', closeChat);
    document.getElementById('pb-clear-btn').addEventListener('click', clearChat);
    document.getElementById('pb-send-btn').addEventListener('click', sendMessage);

    const input = document.getElementById('pb-input');
    input.addEventListener('keydown', handleKey);
    input.addEventListener('input', autoResize);

    document.getElementById('pb-suggestions').addEventListener('click', e => {
      const btn = e.target.closest('.pb-suggest');
      if (btn) sendSuggestion(btn.dataset.text);
    });
  }

  /* ────────────────────────────────────────────────────────
     TOGGLE / OPEN / CLOSE
  ──────────────────────────────────────────────────────── */
  function toggleChat()  { chatOpen ? closeChat() : openChat(); }
  function openChat()    {
    chatOpen = true;
    document.getElementById('plantbot-window').classList.add('open');
    document.getElementById('plantbot-fab').classList.add('open');
    document.getElementById('plantbot-badge').classList.remove('show');
    setTimeout(() => { const i = document.getElementById('pb-input'); if (i) i.focus(); }, 300);
    scrollMessages();
  }
  function closeChat()   {
    chatOpen = false;
    document.getElementById('plantbot-window').classList.remove('open');
    document.getElementById('plantbot-fab').classList.remove('open');
  }

  /* ────────────────────────────────────────────────────────
     MESSAGING
  ──────────────────────────────────────────────────────── */
  function handleKey(e) {
    if (e.key === 'Enter' && !e.shiftKey) { e.preventDefault(); sendMessage(); }
  }

  function autoResize() {
    this.style.height = 'auto';
    this.style.height = Math.min(this.scrollHeight, 80) + 'px';
  }

  function sendSuggestion(text) {
    const input = document.getElementById('pb-input');
    input.value = text;
    sendMessage();
  }

  async function sendMessage() {
    const input = document.getElementById('pb-input');
    const text  = input.value.trim();
    if (!text || isTyping) return;

    input.value = '';
    input.style.height = 'auto';

    appendMsg(text, 'user');
    chatHistory.push({ role: 'user', content: text });

    if (!GROQ_API_KEY) {
      const userKey = prompt('Please enter your Groq API Key to chat with PlantBot (stored locally in browser):');
      if (userKey && userKey.trim()) {
        GROQ_API_KEY = userKey.trim();
        localStorage.setItem('GROQ_API_KEY', GROQ_API_KEY);
      } else {
        appendMsg('⚠️ Groq API key is missing. Set GROQ_API_KEY in your .env file or enter it in the prompt.', 'bot');
        return;
      }
    }

    isTyping = true;
    document.getElementById('pb-send-btn').disabled = true;
    showTypingIndicator();

    const model = document.getElementById('pb-model-sel').value;

    // Keep history to last 20 turns to stay within context limits
    const trimmedHistory = chatHistory.slice(-20);

    try {
      const res = await fetch(GROQ_ENDPOINT, {
        method:  'POST',
        headers: {
          'Content-Type':  'application/json',
          'Authorization': `Bearer ${GROQ_API_KEY}`
        },
        body: JSON.stringify({
          model,
          messages: [
            { role: 'system', content: SYSTEM_PROMPT },
            ...trimmedHistory
          ],
          max_tokens:  700,
          temperature: 0.72
        })
      });

      const data = await res.json();
      removeTypingIndicator();

      if (data.choices && data.choices[0]) {
        const reply = data.choices[0].message.content;
        appendMsg(reply, 'bot');
        chatHistory.push({ role: 'assistant', content: reply });
      } else if (data.error) {
        appendMsg(`⚠️ API error: ${data.error.message}`, 'bot');
      } else {
        appendMsg('Sorry, I could not get a response. Please try again!', 'bot');
      }

    } catch (err) {
      removeTypingIndicator();
      appendMsg('⚠️ Connection error. Check your internet and API key in ai.js.', 'bot');
      console.error('[PlantBot] Fetch error:', err);
    }

    isTyping = false;
    document.getElementById('pb-send-btn').disabled = false;
    document.getElementById('pb-input').focus();
  }

  function clearChat() {
    chatHistory = [];
    document.getElementById('pb-messages').innerHTML = makeWelcomeMsg();
  }

  /* ────────────────────────────────────────────────────────
     DOM HELPERS
  ──────────────────────────────────────────────────────── */
  function appendMsg(text, role) {
    const container = document.getElementById('pb-messages');
    const wrap = document.createElement('div');
    wrap.className = 'pb-msg-wrap ' + role;

    const avatar = role === 'bot' ? '🌿' : '👤';

    // Sanitise & lightly format output
    const safe = escapeHTML(text)
      .replace(/\n/g, '<br>')
      .replace(/\*\*(.*?)\*\*/g, '<strong>$1</strong>')
      .replace(/\*(.*?)\*/g, '<em>$1</em>');

    wrap.innerHTML = `
      <div class="pb-msg-avatar">${avatar}</div>
      <div>
        <div class="pb-msg ${role}">${safe}</div>
        <div class="pb-msg-time">${nowTime()}</div>
      </div>
    `;
    container.appendChild(wrap);
    scrollMessages();
  }

  function showTypingIndicator() {
    const container = document.getElementById('pb-messages');
    const el = document.createElement('div');
    el.className = 'pb-msg-wrap bot';
    el.id = 'pb-typing';
    el.innerHTML = `
      <div class="pb-msg-avatar">🌿</div>
      <div class="pb-msg bot" style="padding:12px 16px">
        <div class="pb-typing-dots"><span></span><span></span><span></span></div>
      </div>
    `;
    container.appendChild(el);
    scrollMessages();
  }

  function removeTypingIndicator() {
    const el = document.getElementById('pb-typing');
    if (el) el.remove();
  }

  function scrollMessages() {
    const el = document.getElementById('pb-messages');
    if (el) el.scrollTop = el.scrollHeight;
  }

  /* ────────────────────────────────────────────────────────
     UTILITY
  ──────────────────────────────────────────────────────── */
  function nowTime() {
    return new Date().toLocaleTimeString('en-IN', { hour: '2-digit', minute: '2-digit' });
  }

  function escapeHTML(str) {
    return str
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;')
      .replace(/"/g, '&quot;');
  }

  function escapeAttr(str) {
    return str.replace(/"/g, '&quot;').replace(/'/g, '&#39;');
  }

  /* ────────────────────────────────────────────────────────
     INIT
  ──────────────────────────────────────────────────────── */
  function init() {
    if (document.getElementById('plantbot-fab')) return; // already mounted
    injectStyles();
    buildHTML();
    console.log('[PlantBot AI] Loaded ✅');
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
  } else {
    init();
  }

})();