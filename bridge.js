try { require('dotenv').config(); } catch (e) {}

const express = require('express');
const axios   = require('axios');
const app     = express();

app.use(express.json());

// ─── CONFIG ──────────────────────────────────────────────
const C_SERVER     = process.env.C_SERVER || 'http://localhost:9000';
const GROQ_API_KEY = process.env.GROQ_API_KEY || '';
const PORT         = process.env.BRIDGE_PORT || 3000;
// ─────────────────────────────────────────────────────────

app.use((req, res, next) => {
  res.header('Access-Control-Allow-Origin', '*');
  res.header('Access-Control-Allow-Headers', 'Content-Type');
  res.header('Access-Control-Allow-Methods', 'GET,POST,OPTIONS');
  if (req.method === 'OPTIONS') return res.sendStatus(200);
  next();
});

// ── LOGIN ─────────────────────────────────────────────────
app.post('/api/login', (req, res) => {
  const { username, password } = req.body;
  if (username === 'admin' && password === 'password')
    return res.json({ success: true });
  res.json({ success: false });
});

// ── PLANTS ────────────────────────────────────────────────
app.get('/api/plants', async (req, res) => {
  try {
    const r = await axios.get(`${C_SERVER}/api/plants`);
    res.json(r.data);
  } catch (e) { res.status(500).json({ error: e.message }); }
});

// ── GARDEN ────────────────────────────────────────────────
app.get('/api/garden', async (req, res) => {
  try {
    const r = await axios.get(`${C_SERVER}/api/garden`);
    res.json(r.data);
  } catch (e) { res.status(500).json({ error: e.message }); }
});

app.post('/api/garden/add', async (req, res) => {
  try {
    const r = await axios.post(`${C_SERVER}/api/garden/add`, req.body);
    res.json(r.data);
  } catch (e) { res.status(500).json({ error: e.message }); }
});

app.post('/api/garden/water', async (req, res) => {
  try {
    const r = await axios.post(`${C_SERVER}/api/garden/water`, req.body);
    res.json(r.data);
  } catch (e) { res.status(500).json({ error: e.message }); }
});

app.post('/api/garden/remove', async (req, res) => {
  try {
    const r = await axios.post(`${C_SERVER}/api/garden/remove`, req.body);
    res.json(r.data);
  } catch (e) { res.status(500).json({ error: e.message }); }
});

// ── CHATBOT ───────────────────────────────────────────────
app.post('/api/chat', async (req, res) => {
  try {
    const { messages } = req.body;

    const formattedMessages = Array.isArray(messages)
      ? messages.map(m =>
          typeof m === 'string'
            ? { role: 'user', content: m }
            : m)
      : [{ role: 'user', content: String(messages) }];

    const r = await axios.post(
      'https://api.groq.com/openai/v1/chat/completions',
      {
        model: 'llama-3.3-70b-versatile',
        max_tokens: 400,
        messages: [
          {
            role: 'system',
            content: 'You are PlantBot, a friendly plant care expert. Give short helpful answers in max 2 sentences.'
          },
          ...formattedMessages.slice(-10)
        ]
      },
      { headers: { Authorization: `Bearer ${GROQ_API_KEY}` } }
    );

    res.json({ reply: r.data.choices[0].message.content });

  } catch (e) {
    console.error('Chat error:', e.message);
    res.status(500).json({ error: e.message });
  }
});

// ── STATS ─────────────────────────────────────────────────
app.get('/api/stats', async (req, res) => {
  try {
    const r = await axios.get(`${C_SERVER}/api/garden`);
    const plants     = r.data;
    const alive      = plants.filter(p => !p.dead && p.mins_left > 0).length;
    const needsWater = plants.filter(p => !p.dead && p.mins_left <= 0).length;
    const dead       = plants.filter(p => p.dead).length;
    res.json({ total: plants.length, alive, needsWater, dead });
  } catch (e) { res.status(500).json({ error: e.message }); }
});

app.listen(PORT, '0.0.0.0', () => {
  console.log(`✅ PlantBridge running → http://localhost:${PORT}`);
});