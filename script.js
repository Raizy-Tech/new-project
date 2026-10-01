const $=(selector,root=document)=>root.querySelector(selector);
const $$=(selector,root=document)=>[...root.querySelectorAll(selector)];

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
},{passive:true});

const year=$('#year');
if(year) year.textContent=new Date().getFullYear();

const observer=new IntersectionObserver(entries=>{
  entries.forEach(entry=>{if(entry.isIntersecting){entry.target.classList.add('is-visible');observer.unobserve(entry.target)}});
},{threshold:.12});
$$('.reveal').forEach(el=>observer.observe(el));

const reduced=window.matchMedia('(prefers-reduced-motion: reduce)').matches;
if(!reduced && window.matchMedia('(pointer:fine)').matches){
  $$('.magnetic').forEach(button=>{
    button.addEventListener('pointermove',event=>{
      const r=button.getBoundingClientRect();
      const x=(event.clientX-r.left-r.width/2)*.14;
      const y=(event.clientY-r.top-r.height/2)*.14;
      button.style.transform=`translate(${x}px,${y}px)`;
    });
    button.addEventListener('pointerleave',()=>button.style.transform='');
  });
  const visual=$('.hero-visual');
  window.addEventListener('pointermove',event=>{
    if(!visual) return;
    const x=(event.clientX/window.innerWidth-.5)*10;
    const y=(event.clientY/window.innerHeight-.5)*10;
    visual.style.transform=`translate3d(${x}px,${y}px,0)`;
  },{passive:true});
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

$$('.service-card a').forEach(link=>{
  link.addEventListener('click',()=>{
    if(navigator.vibrate) navigator.vibrate(8);
  });
});