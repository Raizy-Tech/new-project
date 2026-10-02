# SM Tailoring Production AI Gateway

Updated 2026-10-03.

## What changed

The customer assistant is now built around a provider-neutral AI gateway instead of being tied to one model.

Request path:

`Customer → /api/chat.js → OpenRouter → NVIDIA → LLM7 → Vercel AI Gateway`

Only configured providers are used. If a provider times out, rate-limits, fails, or returns an empty response, the gateway continues to the next configured provider.

## Provider strategy

### OpenRouter — primary
Default model: `openrouter/free`

OpenRouter's free router selects an available free model appropriate to the request. Its current free router supports text and image input and has a 200,000-token context window. OpenRouter also provides a single OpenAI-compatible API across many providers.

### NVIDIA NIM — specialist
Default model: `z-ai/glm-5-3-flash`

NVIDIA exposes an OpenAI-compatible endpoint at `https://integrate.api.nvidia.com/v1/chat/completions`. Current NVIDIA model listings include free endpoints for reasoning, tool use, multimodal and agentic models.

### LLM7 — additional fallback
Kept from the previous implementation. It remains optional and is only used when its key exists.

### Vercel AI Gateway / Qwen — production fallback
Kept from the previous implementation so the existing Qwen integration is not lost.

### Bytez — deliberately separate
Bytez is not placed in the synchronous customer-chat chain yet. Its current free plan provides $1 in credits and access to open models up to 7B, with one open-model request at a time. That makes it more suitable for a future asynchronous specialist/media pipeline than the main customer conversation path.

## Reliability features

- Provider ordering and failover.
- Per-provider timeout.
- Transient failure classification.
- No provider response body is exposed to customers.
- Provider keys stay server-side.
- Maximum user message size: 1500 characters.
- Conversation history limited to 8 messages.
- Provider and model are returned for observability.
- Latency is returned as a non-sensitive diagnostic field.
- `/api/ai-health.js` reports configured providers without exposing credentials.

## Configuration

At least one provider key is required.

Free-first setup:

`OPENROUTER_API_KEY`
`OPENROUTER_MODEL=openrouter/free`

Recommended second provider:

`NVIDIA_API_KEY`
`NVIDIA_MODEL=z-ai/glm-5-3-flash`

Existing providers:

`LLM7_API_KEY`
`AI_GATEWAY_API_KEY`

Optional:

`PUBLIC_APP_URL=https://sm-tailoring.vercel.app`
`AI_PROVIDER_TIMEOUT_MS=12000`

## Product roadmap

1. Image-aware blouse/garment analysis.
2. Separate asynchronous media generation pipeline using specialized providers.
3. Provider health scoring and circuit breakers.
4. Privacy-safe usage metrics.
5. Automated smoke tests when provider credentials are available.
6. Customer-specific knowledge base/RAG for tailoring guidance.
7. Human handoff to WhatsApp when the AI cannot safely answer.

The architecture intentionally separates customer chat from expensive media generation so the product can grow without making the core chat path fragile.
