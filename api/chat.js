const SYSTEM_PROMPT = `
You are the customer assistant for SM Tailoring in Dharmavaram, Andhra Pradesh, India.

Business services:
- Custom blouse stitching
- Blouse alterations
- Women's tailoring
- Dress stitching
- Embroidery
- Custom fitting

Answer questions about these services, measurements, design ideas, alterations, and how to contact the tailor. Be concise and practical. Do not invent prices, delivery dates, appointment availability, policies, or measurements. When a customer asks for an exact price, delivery date, or availability, tell them to contact Sivamma on WhatsApp at +91 91777 87592.

The customer-facing site is informational. For an actual enquiry, encourage the customer to continue on WhatsApp. Do not ask for unnecessary personal information.
`;

export default async function handler(req, res) {
  if (req.method !== "POST") {
    return res.status(405).json({ error: "Method not allowed" });
  }

  const apiKey = process.env.GEMINI_API_KEY;
  if (!apiKey) {
    return res.status(500).json({ error: "AI service is not configured yet." });
  }

  try {
    const body = typeof req.body === "string" ? JSON.parse(req.body) : req.body;
    const message = String(body?.message || "").trim();

    if (!message) {
      return res.status(400).json({ error: "Message is required." });
    }

    if (message.length > 1000) {
      return res.status(400).json({ error: "Message is too long." });
    }

    const response = await fetch(
      "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent?key=" +
        encodeURIComponent(apiKey),
      {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          system_instruction: {
            parts: [{ text: SYSTEM_PROMPT }]
          },
          contents: [
            {
              role: "user",
              parts: [{ text: message }]
            }
          ],
          generationConfig: {
            temperature: 0.4,
            maxOutputTokens: 400
          }
        })
      }
    );

    const data = await response.json();

    if (!response.ok) {
      return res.status(502).json({
        error: "The AI provider returned an error.",
        detail: data?.error?.message || "Unknown provider error."
      });
    }

    const answer =
      data?.candidates?.[0]?.content?.parts
        ?.map(part => part.text || "")
        .join("")
        .trim();

    if (!answer) {
      return res.status(502).json({ error: "The AI provider returned no answer." });
    }

    return res.status(200).json({ answer });
  } catch {
    return res.status(500).json({ error: "Unable to process the request." });
  }
}
