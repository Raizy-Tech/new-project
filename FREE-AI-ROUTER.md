# SM Tailoring free-AI router

The AI assistant tries configured providers in this order: OpenRouter, LLM7, then Vercel AI Gateway / Qwen3.
Only providers with environment variables configured are attempted. Credentials remain server-side.

## Vercel
Add one or more of OPENROUTER_API_KEY, LLM7_API_KEY, and AI_GATEWAY_API_KEY.
Optional model variables: OPENROUTER_MODEL, LLM7_MODEL, AI_GATEWAY_MODEL.

## Connectivity test
On 2026-10-02, the connected build sandbox reached LLM7 but received HTTP 401 without credentials. OpenRouter was also reachable but returned HTTP 401 without credentials. OVHcloud's endpoint hostname did not resolve from that sandbox.

No unauthenticated model response is claimed. A real model response requires credentials where the provider currently requires them.

## Security
Keep provider keys in Vercel environment variables. Do not put them in browser JavaScript or localStorage.