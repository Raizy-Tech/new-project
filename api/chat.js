const SYSTEM_PROMPT = `
You are the customer assistant for SM Tailoring in Dharmavaram, Andhra Pradesh, India.

Business facts:
- Owner: Sivamma
- Location: PRT Street, Dharmavaram, Sri Sathya Sai District, Andhra Pradesh
- Phone/WhatsApp: +91 91777 87592
- Services: custom blouse stitching, blouse alterations, women's tailoring, dress stitching, embroidery, custom fitting.

Rules:
- Help customers understand services and prepare for a tailoring conversation.
- Give practical, conservative guidance about blouse measurements, fit, neckline, sleeves, fabric, alterations and garment preparation.
- Ask useful follow-up questions when underspecified.
- Never invent prices, delivery dates, availability, guarantees, measurements, policies or materials.
- For exact pricing, availability, appointment timing or order-specific decisions, direct the customer to WhatsApp at +91 91777 87592.
- Do not claim to see a garment or photo unless image input is actually provided.
- Keep answers concise and easy to understand.
- Match the customer's language: Telugu for Telugu, English for English, natural mixed language for mixed input.
`;

const PROVIDERS = [
  { name: "openrouter", key: "OPENROUTER_API_KEY", url: "https://openrouter.ai/api/v1/chat/completions", model: process.env.OPENROUTER_MODEL || "openai/gpt-oss-20b:free" },
  { name: "llm7", key: "LLM7_API_KEY", url: "https://api.llm7.io/v1/chat/completions", model: process.env.LLM7_MODEL || "gpt-oss:20b" },
  { name: "ai-gateway", key: "AI_GATEWAY_API_KEY", url: "https://ai-gateway.vercel.sh/v1/chat/completions", model: process.env.AI_GATEWAY_MODEL || "alibaba/qwen-3-235b" }
];

function configuredProviders() {
  return PROVIDERS.filter(p => process.env[p.key]);
}

async function callProvider(provider, messages) {
  const headers = {
    "Authorization": "Bearer " + process.env[provider.key],
    "Content-Type": "application/json"
  };

  if (provider.name === "openrouter") {
    headers["HTTP-Referer"] = "https://sm-tailoring.vercel.app";
    headers["X-Title"] = "SM Tailoring AI Assistant";
  }

  const response = await fetch(provider.url, {
    method: "POST",
    headers,
    body: JSON.stringify({
      model: provider.model,
      messages,
      temperature: 0.35,
      max_tokens: 700
    })
  });

  const data = await response.json().catch(() => ({}));
  if (!response.ok) {
    const error = new Error("Provider returned HTTP " + response.status);
    error.status = response.status;
    error.provider = provider.name;
    throw error;
  }

  const answer = data?.choices?.[0]?.message?.content?.trim();
  if (!answer) throw new Error("Provider returned an empty answer");
  return { answer, provider: provider.name, model: provider.model };
}

export default async function handler(req, res) {
  if (req.method !== "POST") return res.status(405).json({ error: "Method not allowed" });

  try {
    const body = typeof req.body === "string" ? JSON.parse(req.body) : (req.body || {});
    const message = String(body.message || "").trim();
    const history = Array.isArray(body.history) ? body.history : [];

    if (!message) return res.status(400).json({ error: "Message is required." });
    if (message.length > 1500) return res.status(400).json({ error: "Please keep the question under 1500 characters." });

    const safeHistory = history
      .filter(item => item && (item.role === "user" || item.role === "assistant"))
      .slice(-8)
      .map(item => ({ role: item.role, content: String(item.content || "").slice(0, 1500) }));

    const messages = [
      { role: "system", content: SYSTEM_PROMPT },
      ...safeHistory,
      { role: "user", content: message }
    ];

    const providers = configuredProviders();
    if (!providers.length) {
      return res.status(503).json({ error: "No AI provider is configured. Add OPENROUTER_API_KEY, LLM7_API_KEY, or AI_GATEWAY_API_KEY in Vercel." });
    }

    const failures = [];
    for (const provider of providers) {
      try {
        const result = await callProvider(provider, messages);
        return res.status(200).json(result);
      } catch (error) {
        failures.push({ provider: provider.name, status: error.status || 500 });
        console.error("AI provider failed:", provider.name, error);
      }
    }

    return res.status(502).json({
      error: "All configured AI providers are temporarily unavailable. Please try again or continue on WhatsApp.",
      providers: failures.map(f => f.provider)
    });
  } catch (error) {
    console.error("Assistant error:", error);
    return res.status(500).json({ error: "Something went wrong. Please try again or continue on WhatsApp." });
  }
}