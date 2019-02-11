'use strict'

const express = require('express')
const asyncHandler = require('express-async-handler')
const bluebrid = require('bluebird')

const BotApi = require('./lib/botapi')
const config = require('./config')

const ba = new BotApi(config.bot.token, {
  proxyUrl: config.bot.proxy,
  botName: config.bot.bot_name,
})
ba.sendMessageAsync = bluebrid.promisify(ba.sendMessage)

const app = express()

app.use(express.json())

app.post('/receive_message', asyncHandler(async (req, res) => {
  if (!Array.isArray(req.body)) {
    res.status(400).json({ success: false })
    return
  }

  // send messages
  for (const m of req.body) {
    let text = '<b>Service: ' + m.service + '</b>\n'
    if (m.from) {
      text += '<b>From: ' + m.from + '</b>\n'
    }
    if (m.to) {
      text += '<b>To: ' + m.to + '</b>\n'
    }
    text += '\n' + BotApi.encodeHTML(m.text)
    await ba.sendMessageAsync({
      chat_id: config.bot.master_id,
      text,
      parse_mode: 'HTML',
    })
  }

  res.json({ success: true })
}))

app.post('/receive_notification', asyncHandler(async (req, res) => {
  const m = req.body
  let text = '<b>Service: ' + m.service + '</b>\n'
  if (m.header) {
    text += '<b>From: ' + BotApi.encodeHTML(m.header) + '</b>\n'
  }
  if (m.title) {
    text += '<b>' + BotApi.encodeHTML(m.title) + '</b>\n'
  }
  if (m.subtitle) {
    text += '<b>' + BotApi.encodeHTML(m.subtitle) + '</b>\n'
  }
  if (m.message) {
    // mobiletimer
    text += BotApi.encodeHTML(m.message)
  }
  await ba.sendMessageAsync({
    chat_id: config.bot.master_id,
    text,
    parse_mode: 'HTML',
  })

  res.json({ success: true })
}))

const server = app.listen(config.web.port, config.web.hostname, function () {
  const host = server.address().address
  const port = server.address().port
  console.log('Bot server listening at ' + host + ':' + port);
})
