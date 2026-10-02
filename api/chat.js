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
  {
    name: "openrouter",
    key: "OPENROUTER_API_KEY",
    url: "https://openrouter.ai/api/v1/chat/completions",
    model: process.env.OPENROUTER_MODEL || "openrouter/free",
    priority: 1
  },
  {
    name: "nvidia",
    key: "NVIDIA_API_KEY",
    url: "https://integrate.api.nvidia.com/v1/chat/completions",
    model: process.env.NVIDIA_MODEL || "z-ai/glm-5-3-flash",
    priority: 2
  },
  {
    name: "llm7",
    key: "LLM7_API_KEY",
    url: "https://api.llm7.io/v1/chat/completions",
    model: process.env.LLM7_MODEL || "gpt-oss:20b",
    priority: 3
  },
  {
    name: "ai-gateway",
    key: "AI_GATEWAY_API_KEY",
    url: "https://ai-gateway.vercel.sh/v1/chat/completions",
    model: process.env.AI_GATEWAY_MODEL || "alibaba/qwen-3-235b",
    priority: 4
  }
];

const TRANSIENT_STATUSES = new Set([408, 409, 425, 429, 500, 502, 503, 504]);

function configuredProviders() {
  return PROVIDERS.filter(provider => process.env[provider.key]).sort((a, b) => a.priority - b.priority);
}

function withTimeout(ms) {
  const controller = new AbortController();
  const timer = setTimeout(() => controller.abort(), ms);
  return { controller, clear: () => clearTimeout(timer) };
}

async function callProvider(provider, messages) {
  const timeout = withTimeout(Number(process.env.AI_PROVIDER_TIMEOUT_MS || 12000));
  const started = Date.now();

  try {
    const headers = {
      "Authorization": "Bearer " + process.env[provider.key],
      "Content-Type": "application/json",
      "Accept": "application/json"
    };

    if (provider.name === "openrouter") {
      headers["HTTP-Referer"] = process.env.PUBLIC_APP_URL || "https://sm-tailoring.vercel.app";
      headers["X-Title"] = "SM Tailoring AI Assistant";
    }

    const response = await fetch(provider.url, {
      method: "POST",
      headers,
      signal: timeout.controller.signal,
      body: JSON.stringify({
        model: provider.model,
        messages,
        temperature: 0.35,
        max_tokens: 700,
        stream: false
      })
    });

    const data = await response.json().catch(() => ({}));
    const durationMs = Date.now() - started;

    if (!response.ok) {
      const error = new Error("Provider returned HTTP " + response.status);
      error.status = response.status;
      error.provider = provider.name;
      error.transient = TRANSIENT_STATUSES.has(response.status);
      throw error;
    }

    const answer = data?.choices?.[0]?.message?.content?.trim();
    if (!answer) {
      const error = new Error("Provider returned an empty answer");
      error.provider = provider.name;
      error.transient = true;
      throw error;
    }

    return {
      answer,
      provider: provider.name,
      model: data?.model || provider.model,
      latency_ms: durationMs
    };
  } finally {
    timeout.clear();
  }
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
      return res.status(503).json({
        error: "No AI provider is configured. Add OPENROUTER_API_KEY, NVIDIA_API_KEY, LLM7_API_KEY, or AI_GATEWAY_API_KEY in Vercel."
      });
    }

    const failures = [];
    for (const provider of providers) {
      try {
        const result = await callProvider(provider, messages);
        return res.status(200).json(result);
      } catch (error) {
        failures.push({
          provider: provider.name,
          status: error.status || 500,
          transient: error.transient !== false
        });
        console.error("AI provider failed:", {
          provider: provider.name,
          status: error.status || 500,
          transient: error.transient !== false
        });
      }
    }

    return res.status(502).json({
      error: "All configured AI providers are temporarily unavailable. Please try again or continue on WhatsApp.",
      providers: failures.map(failure => failure.provider)
    });
  } catch (error) {
    console.error("Assistant error:", error);
    return res.status(500).json({ error: "Something went wrong. Please try again or continue on WhatsApp." });
  }
}
