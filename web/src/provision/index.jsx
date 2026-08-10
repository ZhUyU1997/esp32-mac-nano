import { render } from 'preact';
import { useEffect } from 'preact/hooks';
import { wifiState, wifiProvisioned } from './store.js';
import { ProvisionView, SuccessView } from './views.jsx';
import './wifi.css';

/* Provisioning page (standalone entry provision.html): fully separate from
 * the remote page (index.html). Pure HTTP (poll /api/status + scan/config/
 * done), zero WebSocket, zero remote-control code. The device httpd serves
 * this page while PROVISIONING/CONNECTING and the remote page otherwise;
 * renders in the iOS CNA WebSheet too (no WS dependency). */
function ProvisionApp() {
  const st = wifiState.value && wifiState.value.state;
  /* Not provisioning (connected/reconnecting/off): the device httpd serves
   * the remote page at "/" — jump there (dev: / is index.html). Excluded:
   * the just-provisioned success page. */
  useEffect(() => {
    if (st && st !== 'PROVISIONING' && st !== 'CONNECTING' &&
        !(st === 'CONNECTED' && wifiProvisioned.value)) {
      location.replace('/');
    }
  }, [st]);
  if (st === 'CONNECTED' && wifiProvisioned.value) {
    /* Just provisioned (within the 20s grace): show the success page (copy IP + close hotspot) */
    return <div className="wifi-shell"><SuccessView /></div>;
  }
  if (st === 'PROVISIONING' || st === 'CONNECTING') {
    return <div className="wifi-shell"><ProvisionView /></div>;
  }
  /* State unknown (page just opened / first poll pending) */
  return (
    <div className="wifi-shell">
      <div className="wifi-center">
        <div className="wifi-spinner" />
        <div className="wifi-center-title">正在连接设备…</div>
      </div>
    </div>
  );
}

render(<ProvisionApp />, document.getElementById('app'));
