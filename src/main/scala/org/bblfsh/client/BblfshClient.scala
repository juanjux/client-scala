package org.bblfsh.client

import github.com.bblfsh.sdk.protocol.generated.{Encoding, ParseRequest, ProtocolServiceGrpc}
import io.grpc.ManagedChannelBuilder


class BblfshClient(host: String, port: Int, maxMsgSize: Int) {

  private val channel = ManagedChannelBuilder
    .forAddress(host, port)
    .usePlaintext(true)
    .maxInboundMessageSize(maxMsgSize)
    .build()

  private val stub = ProtocolServiceGrpc.blockingStub(channel)

  def parse(name: String, content: String, lang: String = "", encoding: Encoding = Encoding.UTF8) = {
    println(s"Parsing file:$name, lang:$lang, content:'${content.substring(0, 20)}'")
    // assume content is already encoded in one of https://github.com/bblfsh/sdk/blob/master/protocol/protocol.go#L68
    val req = ParseRequest(filename = name,
      content = content,
      language = BblfshClient.normalizeLanguage(lang),
      encoding = encoding)
    val parsed = stub.parse(req)
    parsed
  }
}

object BblfshClient {
  val DEFAULT_MAX_MSG_SIZE = 100 * 1024 * 1024

  def apply(host: String, port: Int, maxMsgSize: Int = DEFAULT_MAX_MSG_SIZE): BblfshClient =
    new BblfshClient(host, port, maxMsgSize)

  def normalizeLanguage(lang: String): String = {
    if (lang == null) {
      return ""
    }

    lang
      .toLowerCase
      .replace(" ", "-")
      .replace("+", "p")
      .replace("#", "sharp")
  }
}

