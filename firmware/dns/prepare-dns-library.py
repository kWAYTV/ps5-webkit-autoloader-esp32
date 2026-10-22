#!/usr/bin/env python3
"""Copy Arduino-ESP32 2.0.11 DNSServer with a project DNS allowlist hook."""

import sys
from pathlib import Path


def replace_once(text: str, old: str, new: str) -> str:
    if text.count(old) != 1:
        raise RuntimeError(f"DNSServer source changed; expected one occurrence of {old!r}")
    return text.replace(old, new, 1)


def main() -> None:
    if len(sys.argv) != 3:
        raise SystemExit("usage: prepare-dns-library.py CORE_DNS_DIR OUTPUT_DIR")
    original, output = map(Path, sys.argv[1:])
    properties = (original / "library.properties").read_text()
    if "version=2.0.0\n" not in properties:
        raise RuntimeError("DNS patch needs the DNSServer from Arduino-ESP32 2.0.11")

    source = (original / "src/DNSServer.cpp").read_text()
    header = (original / "src/DNSServer.h").read_text()
    source = replace_once(source, "#define DEBUG_ESP_DNS\n",
                          "// The sketch owns the allowlist; all other names receive NXDOMAIN.\n"
                          "extern bool portalDnsAllowed(const String &domain);\n"
                          "#ifndef USB_DEBUG\n#define USB_DEBUG 0\n#endif\n")
    source = replace_once(source, '  domainName.replace("www.", "");\n', "")
    source = replace_once(
        source,
        '''    if (_dnsHeader->QR == DNS_QR_QUERY &&
        _dnsHeader->OPCode == DNS_OPCODE_QUERY &&
        requestIncludesOnlyOneQuestion() &&
        (_domainName == "*" || getDomainNameWithoutWwwPrefix() == _domainName)
       )
    {
      replyWithIP();
    }
    else if (_dnsHeader->QR == DNS_QR_QUERY)
    {
      replyWithCustomCode();
    }
''',
        '''    if (_dnsHeader->QR == DNS_QR_QUERY)
    {
      bool valid = _dnsHeader->OPCode == DNS_OPCODE_QUERY &&
                   requestIncludesOnlyOneQuestion();
      String domain = valid ? getDomainNameWithoutWwwPrefix() : String("<invalid>");
      uint16_t qtype = valid ? ntohs(_dnsQuestion->QType) : 0;
      bool allowed = valid && ntohs(_dnsQuestion->QClass) == DNS_CLASS_IN &&
                     portalDnsAllowed(domain);
      const char *result;
      if (allowed && (qtype == DNS_TYPE_A || qtype == 255))
      {
        replyWithIP();
        result = "192.168.4.1";
      }
      else if (allowed)
      {
        replyWithNoData();
        result = "NO DATA";
      }
      else
      {
        replyWithCustomCode(valid);
        result = "NXDOMAIN";
      }
#if USB_DEBUG
      char type[16];
      if (qtype == DNS_TYPE_A) strcpy(type, "A");
      else if (qtype == DNS_TYPE_AAAA) strcpy(type, "AAAA");
      else snprintf(type, sizeof(type), "TYPE%u", (unsigned)qtype);
      Serial.printf("[DNS] %s %s -> %s\\n", domain.c_str(), type, result);
#endif
    }
''')
    source = replace_once(
        source,
        '''  return ntohs(_dnsHeader->QDCount) == 1 &&
         _dnsHeader->ANCount == 0 &&
         _dnsHeader->NSCount == 0 &&
         _dnsHeader->ARCount == 0;''',
        '''  // The optional EDNS OPT record is ignored and not copied into replies.
  return ntohs(_dnsHeader->QDCount) == 1 &&
         _dnsHeader->ANCount == 0 &&
         _dnsHeader->NSCount == 0;''')
    source = replace_once(source,
                          '  _dnsHeader->ANCount = _dnsHeader->QDCount;\n',
                          '  _dnsHeader->ANCount = _dnsHeader->QDCount;\n'
                          '  _dnsHeader->ARCount = 0; // The OPT record is not copied.\n')
    source = replace_once(source,
                          '''  #ifdef DEBUG_ESP_DNS
    DEBUG_OUTPUT.printf("DNS responds: %s for %s\\n",
            IPAddress(_resolvedIP).toString().c_str(), getDomainNameWithoutWwwPrefix().c_str() );
  #endif  
''', '')
    source = replace_once(source,
                          'void DNSServer::replyWithCustomCode()\n{',
                          '''void DNSServer::replyWithNoData()
{
  _dnsHeader->QR = DNS_QR_RESPONSE;
  _dnsHeader->RCode = 0;
  _dnsHeader->ANCount = 0;
  _dnsHeader->NSCount = 0;
  _dnsHeader->ARCount = 0;
  _udp.beginPacket(_udp.remoteIP(), _udp.remotePort());
  _udp.write((unsigned char*)_dnsHeader, DNS_HEADER_SIZE);
  _udp.write(_dnsQuestion->QName, _dnsQuestion->QNameLength);
  _udp.write((unsigned char*)&_dnsQuestion->QType, 2);
  _udp.write((unsigned char*)&_dnsQuestion->QClass, 2);
  _udp.endPacket();
}

void DNSServer::replyWithCustomCode(bool includeQuestion)
{''')
    source = replace_once(source,
                          '''  _dnsHeader->QDCount = 0;

  _udp.beginPacket''',
                          '''  _dnsHeader->QDCount = htons(includeQuestion ? 1 : 0);
  _dnsHeader->ANCount = 0;
  _dnsHeader->NSCount = 0;
  _dnsHeader->ARCount = 0;

  _udp.beginPacket''')
    source = replace_once(source,
                          '''  _udp.write((unsigned char*)_dnsHeader, sizeof(DNSHeader));
  _udp.endPacket();
}''',
                          '''  _udp.write((unsigned char*)_dnsHeader, DNS_HEADER_SIZE);
  if (includeQuestion) {
    _udp.write(_dnsQuestion->QName, _dnsQuestion->QNameLength);
    _udp.write((unsigned char*)&_dnsQuestion->QType, 2);
    _udp.write((unsigned char*)&_dnsQuestion->QClass, 2);
  }
  _udp.endPacket();
}''')
    header = replace_once(header,
                          '    void replyWithIP();\n',
                          '    void replyWithIP();\n    void replyWithNoData();\n')
    header = replace_once(header,
                          '    void replyWithCustomCode();',
                          '    void replyWithCustomCode(bool includeQuestion);')

    (output / "src").mkdir(parents=True, exist_ok=True)
    (output / "library.properties").write_text(properties)
    (output / "src/DNSServer.h").write_text(header)
    (output / "src/DNSServer.cpp").write_text(source)
    (output / ".ready").touch()


if __name__ == "__main__":
    main()
