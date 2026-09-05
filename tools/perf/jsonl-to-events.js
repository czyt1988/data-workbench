// jsonl-to-events.js — diagnostic harness (throwaway, will be removed)
// Faithfully replicates DAAgentWebChannel::loadHistory merging logic (C++ side)
// so the browser harness receives exactly what chat.js receives in production.
// Usage: node jsonl-to-events.js <session.jsonl> <out-events.js>
'use strict';
const fs = require('fs');
const src = process.argv[2];
const out = process.argv[3];
if (!src || !out) { console.error('usage: node jsonl-to-events.js <in.jsonl> <out.js>'); process.exit(1); }

const lines = fs.readFileSync(src, 'utf8').split('\n');
const uiEvents = [];
const pendingToolCalls = new Map(); // id -> {toolName, args}
let skippedUnpaired = 0;

for (const line of lines) {
  const t = line.trim();
  if (!t) continue;
  let rec;
  try { rec = JSON.parse(t); } catch (e) { continue; } // parseLineTolerant skip
  const type = rec.type;
  const msg = rec.message || {};
  if (type === 'user' || type === 'usage' || type === 'summary') {
    uiEvents.push(rec); // passthrough
  } else if (type === 'assistant') {
    const tcs = Array.isArray(msg.tool_calls) ? msg.tool_calls : [];
    if (tcs.length > 0) {
      const content = typeof msg.content === 'string' ? msg.content : '';
      if (content.length > 0) {
        uiEvents.push({ type: 'assistant', message: msg });
      }
      for (const tc of tcs) {
        pendingToolCalls.set(tc.id, { toolName: tc.name, args: (tc.args && typeof tc.args === 'object') ? tc.args : {} });
      }
    } else {
      uiEvents.push(rec); // plain-text assistant
    }
  } else if (type === 'tool_result') {
    const id = msg.tool_call_id;
    const meta = pendingToolCalls.get(id);
    if (!meta) { skippedUnpaired++; continue; }
    pendingToolCalls.delete(id);
    const isAsk = meta.toolName === 'ask_user';
    const ev = { type: isAsk ? 'question' : 'tool', toolName: meta.toolName, args: meta.args, toolCallId: id };
    if (isAsk) {
      ev.result = { answer: typeof msg.content === 'string' ? msg.content : '' };
    } else {
      // parseJsonStr: empty or invalid -> {}
      const c = typeof msg.content === 'string' ? msg.content : '';
      if (c.length > 0) {
        try { ev.result = JSON.parse(c); } catch (e) { ev.result = {}; }
      } else { ev.result = {}; }
    }
    uiEvents.push(ev);
  }
  // other types skipped (matches C++)
}
fs.writeFileSync(out, 'window.__EVENTS = ' + JSON.stringify(uiEvents) + ';\n');
const byType = {};
for (const e of uiEvents) byType[e.type] = (byType[e.type] || 0) + 1;
console.log('events: ' + uiEvents.length + '  byType: ' + JSON.stringify(byType) + '  unpairedToolResults: ' + skippedUnpaired);
