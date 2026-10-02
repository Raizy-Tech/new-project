# SM Tailoring AI Assistant

The website uses Qwen3-235B-A22B through Vercel AI Gateway for the `/api/chat` serverless endpoint.

## Vercel setup

1. Create an AI Gateway API key in Vercel.
2. Add `AI_GATEWAY_API_KEY` to the SM Tailoring project for Production and Preview.
3. Redeploy the project.
4. Open `/ai-assistant.html` and test the quick questions.

Keep the key server-side. Never place it in client-side HTML or JavaScript.

## Assistant behavior

The assistant knows SM Tailoring's documented services and contact details. It is instructed not to invent prices, delivery dates, availability, guarantees, measurements or order decisions. Exact commercial details are routed to WhatsApp.

## Model

The API uses Vercel AI Gateway model ID `alibaba/qwen-3-235b`, which routes to Qwen3 235B A22B.
