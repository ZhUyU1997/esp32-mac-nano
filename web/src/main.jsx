import { render } from 'preact';
import 'cropperjs/dist/cropper.min.css';
import { App } from './app.jsx';
import './store.js';   /* side effect: start WS connection + floppy status polling */

render(<App />, document.getElementById('app'));
