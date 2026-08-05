// Pre-generate HTTPS cert for dashi-ppt preview using openssl inside WSL
import { execFileSync } from 'node:child_process';
import { mkdirSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { getLocalHostname, getLanIps } from 'file:///c:/Users/phi.vu/.agents/skills/dashi-ppt/project/scripts/preview/network.mjs';
import { renderCertificateMeta, renderOpenSslConfig } from 'file:///c:/Users/phi.vu/.agents/skills/dashi-ppt/project/scripts/preview/tls.mjs';

const ROOT = 'c:/Users/phi.vu/.agents/skills/dashi-ppt/project';
const certDir = path.join(ROOT, 'output/https-preview');
mkdirSync(certDir, { recursive: true });

const localHostname = getLocalHostname();
const lanIps = getLanIps();
const names = ['localhost', `${localHostname}.local`, ...lanIps];
const meta = renderCertificateMeta(names);
const config = renderOpenSslConfig(names, localHostname);

const configFile = path.join(certDir, 'openssl.cnf');
writeFileSync(configFile, config);

const toWsl = (p) => execFileSync('wsl', ['wslpath', '-a', p.replace(/\\/g, '/')], { encoding: 'utf8' }).trim();

const keyFile = path.join(certDir, 'localhost-key.pem');
const certFile = path.join(certDir, 'localhost-cert.pem');

execFileSync('wsl', [
  'openssl', 'req', '-x509', '-newkey', 'rsa:2048', '-nodes', '-sha256', '-days', '365',
  '-keyout', toWsl(keyFile),
  '-out', toWsl(certFile),
  '-config', toWsl(configFile),
  '-extensions', 'v3_req',
], { stdio: 'inherit' });

writeFileSync(path.join(certDir, 'cert-meta.json'), meta + '\n');
console.log('Cert generated for names:', names.join(', '));
