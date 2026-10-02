# AI Provider Master Note

Updated: 2026-10-03

## Decision

Primary provider: OpenRouter.

Specialist provider: NVIDIA NIM.

Additional fallbacks: LLM7 and Vercel AI Gateway.

Future specialist/media provider: Bytez.

## Why

OpenRouter provides a unified OpenAI-compatible API and a free-model router that can dynamically choose among available free models. Its current free router supports text and image input with a 200k context window.

NVIDIA provides a separate OpenAI-compatible API and currently exposes free endpoints for reasoning, multimodal, tool-use and agentic models. This gives the product a second independent provider rather than relying entirely on one routing platform.

Bytez has a very large model catalog and broad modalities, but its current free plan is constrained to open models up to 7B and one open-model request at a time. It is therefore better for future asynchronous specialist workloads than customer chat.

## Current product architecture

Customer
→ /api/chat.js
→ OpenRouter free router
→ NVIDIA
→ LLM7
→ Vercel AI Gateway / Qwen

Only configured providers are attempted.

## Important environment variables

OPENROUTER_API_KEY
OPENROUTER_MODEL=openrouter/free

NVIDIA_API_KEY
NVIDIA_MODEL=z-ai/glm-5-3-flash

LLM7_API_KEY
LLM7_MODEL=gpt-oss:20b

AI_GATEWAY_API_KEY
AI_GATEWAY_MODEL=alibaba/qwen-3-235b

AI_PROVIDER_TIMEOUT_MS=12000

## Security rule

Never put provider API keys in frontend JavaScript, localStorage, public GitHub files, or client-side configuration.

## Future expansion

- Image-aware blouse/garment analysis
- RAG over tailoring knowledge
- Provider health scoring/circuit breakers
- Privacy-safe usage telemetry
- Automated provider smoke tests
- Bytez media/specialist pipeline
- Higgsfield generation pipeline for marketing media
- Human WhatsApp handoff for uncertain commercial questions
