import fs from 'fs';
import { execSync } from 'child_process';

const layouts = ['theme03_page004','theme03_page006','theme03_page007','theme03_page014','theme03_page065','theme03_page034','theme03_page047','theme03_page040','theme03_page035','theme03_page077','theme03_page036','theme03_page028'];
const out = execSync(`node "c:/Users/phi.vu/.agents/skills/dashi-ppt/project/scripts/inspect-layout.mjs" --compact ${layouts.join(' ')}`, { maxBuffer: 64e6 }).toString();
const d = JSON.parse(out);
const res = d.layouts.map(l => ({
  layout: l.layout,
  contentLocked: l.contentLocked || false,
  propShapes: l.propShapes,
  budgets: Object.fromEntries(Object.entries(l.copyBudgets || {}).map(([k, v]) => [k, v.maxChars])),
  arrays: (l.arrayMeta || []).map(a => ({ key: a.key, visible: a.defaultVisibleCount, max: a.maxCount })),
  decorativeKeys: l.decorativeKeys || [],
}));
fs.mkdirSync('tmp_output', { recursive: true });
fs.writeFileSync('tmp_output/dashi_contracts.json', JSON.stringify(res, null, 1));
console.log('written', res.length);
