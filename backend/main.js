/***************************************************************************
 *  
 *  Project:       mallocAi - AI-guided memory allocation layer
 *  Description:   Node.js backend using Google Gemini API to respond
 *                 to memory allocation prompts over HTTP.
 *
 *  Author:        almightynan
 *  License:       MIT (see LICENSE file for details)
 *
 *  WARNING: This server processes open-ended input and returns
 *  responses intended for direct use in low-level C memory routines.
 *  Validate and sanitize outputs before applying in critical code.
 * 
 ***************************************************************************/

import express from "express";
import { GoogleGenerativeAI } from "@google/generative-ai";
import dotenv from "dotenv";
dotenv.config();

const app = express();
app.use(express.json());

app.post("/ai", async (req, res) => {
  const { prompt } = req.body;
  if (!prompt) return res.status(400).json({ error: "Prompt required" });

  try {
    const genAI = new GoogleGenerativeAI(process.env.GOOGLE_API_KEY);
    const model = genAI.getGenerativeModel({ model: "gemini-2.0-flash" });
    const result = await model.generateContent(process.env.PROMPT.replace("{prompt}", prompt)); // figure it out yourself, nerd :P
    const text = (await result.response).text();

    res.json({ text });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

const PORT = process.env.PORT || 3000;
app.listen(PORT);
console.log(`Server running on port ${PORT}`);