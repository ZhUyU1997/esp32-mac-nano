import { keyEv, capsLocked } from './store.js';
import './styles/keyboard.css';

/* HID usage tables */
const U = { esc: 0x29,'1': 0x1e,'2': 0x1f,'3': 0x20,'4': 0x21,'5': 0x22,'6': 0x23,'7': 0x24,'8': 0x25,'9': 0x26,'0': 0x27,'-': 0x2d,'=': 0x2e,
  tab: 0x2b,q: 0x14,w: 0x1a,e: 0x08,r: 0x15,t: 0x17,y: 0x1c,u: 0x18,i: 0x0c,o: 0x12,p: 0x13,'[': 0x2f,']': 0x30,'\\': 0x31,
  cap: 0x39,a: 0x04,s: 0x16,d: 0x07,f: 0x09,g: 0x0a,h: 0x0b,j: 0x0d,k: 0x0e,l: 0x0f,';': 0x33,"'": 0x34,ret: 0x28,
  ls: 0xe1,z: 0x1d,x: 0x1b,c: 0x06,v: 0x19,b: 0x05,n: 0x11,m: 0x10,',': 0x36,'.': 0x37,'/': 0x38,rs: 0xe5,
  lc: 0xe0,la: 0xe2,lg: 0xe3,sp: 0x2c,bksp: 0x2a,left: 0x50,up: 0x52,down: 0x51,right: 0x4f };

/* keyboard layout */
const KB = [
  [['esc','esc'],['1'],['2'],['3'],['4'],['5'],['6'],['7'],['8'],['9'],['0'],['-'],['='],['bksp','⌫','wide']],
  [['tab','tab','wide'],['q','Q'],['w','W'],['e','E'],['r','R'],['t','T'],['y','Y'],['u','U'],['i','I'],['o','O'],['p','P'],['['],[']'],['\\']],
  [['cap','caps'],['a','A'],['s','S'],['d','D'],['f','F'],['g','G'],['h','H'],['j','J'],['k','K'],['l','L'],[';'],["'"],['ret','↵','wide']],
  [['ls','⇧','wide'],['z','Z'],['x','X'],['c','C'],['v','V'],['b','B'],['n','N'],['m','M'],[','],['.'],['/'],['rs','⇧','wide']],
  [['lc','⌃','mod'],['la','⌥','mod'],['lg','⌘','mod'],['sp','空格','space'],['left','←'],['up','↑'],['down','↓'],['right','→']]
];

export function Keyboard() {
  return (
    <div className="kb" id="kb">
      {KB.map((row, ri) => (
        <div className="krow" key={ri}>
          {row.map(([usage, label, cls], ci) => {
            if (usage === 'cap') {
              /* CapsLock: toggle（由信号驱动高亮） */
              return (
                <button key={ci} id="capKey"
                  className={'key' + (capsLocked.value ? ' on' : '')}
                  onPointerDown={e => {
                    e.preventDefault();
                    capsLocked.value = !capsLocked.value;
                    keyEv(U[usage], capsLocked.value);
                  }}>
                  {label || usage}
                </button>
              );
            }
            return (
              <button key={ci} className={'key' + (cls ? ' ' + cls : '')}
                onPointerDown={e => { e.preventDefault(); keyEv(U[usage], true); e.currentTarget.classList.add('on'); }}
                onPointerUp={e => { keyEv(U[usage], false); e.currentTarget.classList.remove('on'); }}
                onPointerLeave={e => { if (e.currentTarget.classList.contains('on')) { keyEv(U[usage], false); e.currentTarget.classList.remove('on'); } }}>
                {label || usage}
              </button>
            );
          })}
        </div>
      ))}
    </div>
  );
}
