import { render } from 'preact';
import 'cropperjs/dist/cropper.min.css';
import { App } from './app.jsx';
import { ensureWs } from './store.js';

/* Remote page (standalone entry index.html): WS always on (keys/mouse/shot/
 * floppy). The provisioning page is provision.html, served by httpd based
 * on wifi state. */
ensureWs();   /* side effect: start WS connection + floppy status polling */

render(<App />, document.getElementById('app'));
