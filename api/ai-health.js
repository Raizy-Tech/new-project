const PROVIDERS = [
  { name: "openrouter", key: "OPENROUTER_API_KEY", model: process.env.OPENROUTER_MODEL || "openrouter/free" },
  { name: "nvidia", key: "NVIDIA_API_KEY", model: process.env.NVIDIA_MODEL || "z-ai/glm-5-3-flash" },
  { name: "llm7", key: "LLM7_API_KEY", model: process.env.LLM7_MODEL || "gpt-oss:20b" },
  { name: "ai-gateway", key: "AI_GATEWAY_API_KEY", model: process.env.AI_GATEWAY_MODEL || "alibaba/qwen-3-235b" }
];

export default function handler(req, res) {
  if (req.method !== "GET") return res.status(405).json({ error: "Method not allowed" });

  const configured = PROVIDERS
    .filter(provider => Boolean(process.env[provider.key]))
    .map(provider => ({
      name: provider.name,
      model: provider.model,
      configured: true
    }));

  return res.status(200).json({
    service: "SM Tailoring AI Gateway",
    status: configured.length ? "configured" : "not_configured",
    providers: configured,
    free_first: true
  });
}
