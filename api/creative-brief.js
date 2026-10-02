const PROVIDERS=[
 {name:"openrouter",key:"OPENROUTER_API_KEY",url:"https://openrouter.ai/api/v1/chat/completions",model:process.env.OPENROUTER_MODEL||"openrouter/free",priority:1},
 {name:"nvidia",key:"NVIDIA_API_KEY",url:"https://integrate.api.nvidia.com/v1/chat/completions",model:process.env.NVIDIA_MODEL||"z-ai/glm-5-3-flash",priority:2},
 {name:"llm7",key:"LLM7_API_KEY",url:"https://api.llm7.io/v1/chat/completions",model:process.env.LLM7_MODEL||"gpt-oss:20b",priority:3},
 {name:"ai-gateway",key:"AI_GATEWAY_API_KEY",url:"https://ai-gateway.vercel.sh/v1/chat/completions",model:process.env.AI_GATEWAY_MODEL||"alibaba/qwen-3-235b",priority:4}
];
const SYSTEM=`You are the marketing creative director for SM Tailoring in Dharmavaram, Andhra Pradesh, India.
Create original, practical marketing content for local tailoring customers.
Business: custom blouse stitching, blouse alterations, women's tailoring, dress stitching, embroidery, custom fitting.
Never invent prices, discounts, delivery promises, availability, customer testimonials, guarantees, or business policies.
Never invent customer stories or claim a result happened unless supplied in the brief.
The output must be concise and ready to edit.
Return exactly these sections:
CAMPAIGN ANGLE
HOOKS (3)
CAPTION
SHORT VIDEO SCRIPT
CTA
VISUAL DIRECTION
Match the requested language naturally. For Telugu + English, use natural mixed language rather than literal translation.`;

function providers(){return PROVIDERS.filter(p=>process.env[p.key]).sort((a,b)=>a.priority-b.priority)}
function timeout(ms){const c=new AbortController();const t=setTimeout(()=>c.abort(),ms);return{c,clear:()=>clearTimeout(t)}}
async function call(p,messages){
 const t=timeout(Number(process.env.AI_PROVIDER_TIMEOUT_MS||12000));
 try{
  const headers={"Authorization":"Bearer "+process.env[p.key],"Content-Type":"application/json","Accept":"application/json"};
  if(p.name==="openrouter"){headers["HTTP-Referer"]=process.env.PUBLIC_APP_URL||"https://sm-tailoring.vercel.app";headers["X-Title"]="SM Tailoring Creative Studio"}
  const r=await fetch(p.url,{method:"POST",headers,signal:t.c.signal,body:JSON.stringify({model:p.model,messages,temperature:.55,max_tokens:1000,stream:false})});
  const d=await r.json().catch(()=>({}));
  if(!r.ok)throw new Error("provider_http_"+r.status);
  const answer=d?.choices?.[0]?.message?.content?.trim();
  if(!answer)throw new Error("empty_provider_response");
  return{answer,provider:p.name,model:d?.model||p.model};
 }finally{t.clear()}
}
export default async function handler(req,res){
 if(req.method!=="POST")return res.status(405).json({error:"Method not allowed"});
 try{
  const body=typeof req.body==="string"?JSON.parse(req.body):(req.body||{});
  const fields=["service","audience","platform","language","tone","idea"];
  const brief=Object.fromEntries(fields.map(k=>[k,String(body[k]||"").trim().slice(0,700)]));
  if(!brief.service)return res.status(400).json({error:"Service is required."});
  const configured=providers();
  if(!configured.length)return res.status(503).json({error:"No AI provider is configured."});
  const user=`Create a campaign for:
Service: ${brief.service}
Audience: ${brief.audience||"local customers"}
Platform: ${brief.platform||"Instagram"}
Language: ${brief.language||"English"}
Tone: ${brief.tone||"elegant and personal"}
Idea: ${brief.idea||"Create an original introduction to this tailoring service."}`;
  for(const p of configured){try{return res.status(200).json(await call(p,[{role:"system",content:SYSTEM},{role:"user",content:user}]))}catch(e){console.error("Creative provider failed:",p.name)}}
  return res.status(502).json({error:"Creative providers are temporarily unavailable."});
 }catch(e){console.error("Creative brief error:",e);return res.status(500).json({error:"Something went wrong."})}
}