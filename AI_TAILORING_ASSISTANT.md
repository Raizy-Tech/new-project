# SM Tailoring AI Assistant

This project can use a free-tier LLM as a customer-facing tailoring assistant without exposing the API key in browser JavaScript.

## Recommended provider
Google Gemini is listed in the referenced free-LLM catalog with a free API tier and OpenAI-compatible setup. This project should use a server-side Vercel function and keep `GEMINI_API_KEY` in Vercel Environment Variables.

## Planned flow
1. Customer opens the assistant.
2. Browser sends the customer's message to `/api/chat`.
3. The Vercel function calls Gemini using `GEMINI_API_KEY`.
4. The assistant answers only about SM Tailoring services, fitting, alterations, embroidery, dress/blouse enquiries, and how to contact the business.
5. The UI can hand the customer off to WhatsApp for an actual enquiry or booking.

## Security rule
Never put `GEMINI_API_KEY` in `index.html`, `script.js`, or any client-side JavaScript. Use a Vercel server-side environment variable.

## Initial system prompt
You are the customer assistant for SM Tailoring in Dharmavaram, Andhra Pradesh. Give concise, practical answers about custom blouse stitching, blouse alterations, women's tailoring, dress stitching, embroidery, custom fitting, measurements, and contacting the tailor. Do not invent prices, delivery dates, availability, policies, or measurements. If the customer asks for a price or exact availability, tell them to contact Sivamma on WhatsApp. Be polite and concise. Do not collect unnecessary personal information.
