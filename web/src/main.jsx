import { render } from 'preact';
import 'cropperjs/dist/cropper.min.css';
import { App } from './app.jsx';
import './store.js';   /* 副作用：启动 WS 连接 + floppy 状态轮询 */

render(<App />, document.getElementById('app'));
