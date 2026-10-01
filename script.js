const $=(selector,root=document)=>root.querySelector(selector);
const $$=(selector,root=document)=>[...root.querySelectorAll(selector)];
const reduced=window.matchMedia('(prefers-reduced-motion: reduce)').matches;
const finePointer=window.matchMedia('(pointer:fine)').matches;

document.body.classList.add('loading');
window.addEventListener('load',()=>{document.body.classList.remove('loading');window.setTimeout(()=>$('.page-loader')?.classList.add('done'),500)},{once:true});
window.addEventListener('pageshow',()=>$('.page-loader')?.classList.add('done'));

const menuButton=$('.menu-toggle');
const nav=$('.nav');
menuButton?.addEventListener('click',()=>{
  const open=nav.classList.toggle('open');
  menuButton.setAttribute('aria-expanded',String(open));
  menuButton.setAttribute('aria-label',open?'Close navigation':'Open navigation');
});
$$('.nav a').forEach(link=>link.addEventListener('click',()=>{
  nav.classList.remove('open');
  menuButton?.setAttribute('aria-expanded','false');
  menuButton?.setAttribute('aria-label','Open navigation');
}));

const header=$('.site-header');
let lastScroll=window.scrollY;
window.addEventListener('scroll',()=>{
  const current=window.scrollY;
  header?.classList.toggle('scrolled',current>20);
  if(current>140 && current>lastScroll && !nav?.classList.contains('open')) header?.classList.add('hidden');
  else header?.classList.remove('hidden');
  lastScroll=current;
  const max=document.documentElement.scrollHeight-window.innerHeight;
  const bar=$('.scroll-progress i');
  if(bar) bar.style.width=`${max>0?(current/max)*100:0}%`;
},{passive:true});

const year=$('#year');
if(year) year.textContent=new Date().getFullYear();

const observer=new IntersectionObserver(entries=>{
  entries.forEach(entry=>{
    if(entry.isIntersecting){entry.target.classList.add('is-visible');observer.unobserve(entry.target)}
  });
},{threshold:.12});
$$('.reveal').forEach(el=>observer.observe(el));

if(!reduced && finePointer){
  const glow=$('.cursor-glow');
  window.addEventListener('pointermove',event=>{
    if(glow){glow.style.left=`${event.clientX}px`;glow.style.top=`${event.clientY}px`}
  },{passive:true});
  $$('.magnetic').forEach(button=>{
    button.addEventListener('pointermove',event=>{
      const r=button.getBoundingClientRect();
      const x=(event.clientX-r.left-r.width/2)*.14;
      const y=(event.clientY-r.top-r.height/2)*.14;
      button.style.transform=`translate(${x}px,${y}px)`;
    });
    button.addEventListener('pointerleave',()=>button.style.transform='');
  });
  $$('.service-card[data-tilt]').forEach(card=>{
    card.addEventListener('pointermove',event=>{
      const r=card.getBoundingClientRect();
      const x=(event.clientX-r.left)/r.width-.5;
      const y=(event.clientY-r.top)/r.height-.5;
      card.style.transform=`perspective(900px) rotateX(${y*-5}deg) rotateY(${x*5}deg) translateY(-5px)`;
    });
    card.addEventListener('pointerleave',()=>card.style.transform='');
  });
  const visual=$('.hero-visual');
  window.addEventListener('pointermove',event=>{
    if(!visual)return;
    const x=(event.clientX/window.innerWidth-.5)*12;
    const y=(event.clientY/window.innerHeight-.5)*12;
    visual.style.transform=`translate3d(${x}px,${y}px,0)`;
  },{passive:true});
}

const canvas=$('#thread-canvas');
if(canvas && !reduced){
  const ctx=canvas.getContext('2d');
  let width=0,height=0,dpr=1;
  const points=[];
  const resize=()=>{
    dpr=Math.min(window.devicePixelRatio||1,2);
    width=canvas.clientWidth;height=canvas.clientHeight;
    canvas.width=width*dpr;canvas.height=height*dpr;
    ctx.setTransform(dpr,0,0,dpr,0,0);
    points.length=0;
    const count=Math.min(34,Math.max(18,Math.floor(width/34)));
    for(let i=0;i<count;i++)points.push({x:Math.random()*width,y:Math.random()*height,vx:(Math.random()-.5)*.18,vy:(Math.random()-.5)*.18,phase:Math.random()*Math.PI*2});
  };
  const draw=()=>{
    ctx.clearRect(0,0,width,height);
    for(let i=0;i<points.length;i++){
      const p=points[i];
      p.phase+=.005;p.x+=p.vx+Math.sin(p.phase)*.035;p.y+=p.vy+Math.cos(p.phase)*.035;
      if(p.x<-20)p.x=width+20;if(p.x>width+20)p.x=-20;if(p.y<-20)p.y=height+20;if(p.y>height+20)p.y=-20;
      for(let j=i+1;j<points.length;j++){
        const q=points[j];const dist=Math.hypot(p.x-q.x,p.y-q.y);
        if(dist<190){ctx.strokeStyle=`rgba(91,31,50,${(1-dist/190)*.08})`;ctx.lineWidth=1;ctx.beginPath();ctx.moveTo(p.x,p.y);ctx.lineTo(q.x,q.y);ctx.stroke()}
      }
    }
    requestAnimationFrame(draw);
  };
  resize();window.addEventListener('resize',resize,{passive:true});draw();
}

$('#enquiry-form')?.addEventListener('submit',event=>{
  event.preventDefault();
  const form=new FormData(event.currentTarget);
  const name=String(form.get('name')||'').trim();
  const service=String(form.get('service')||'').trim();
  const message=String(form.get('message')||'').trim();
  const body=['Hello SM Tailoring,','My name is '+name+'.','I am enquiring about: '+service+'.',message?'Details: '+message:''].filter(Boolean).join('\n');
  window.open('https://wa.me/919177787592?text='+encodeURIComponent(body),'_blank','noopener');
});
$$('.service-card a').forEach(link=>link.addEventListener('click',()=>{if(navigator.vibrate)navigator.vibrate(8)}));