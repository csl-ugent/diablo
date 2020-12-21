/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "diabloobject_dwarf.h"

/* read in an ULEB128-encoded integer from the given location */
DwarfEncodedULEB128
DwarfReadULEB128FromSection(t_section *section, t_address offset, t_uint32& n_bytes)
{
  DwarfEncodedULEB128 ret;

  /* only read up to the number of supported bytes */
  while (true)
  {
    /* read the next byte */
    t_uint8 byte = SectionGetData8(section, offset);
    ret.push_back(byte);
    offset = AddressAddUint32(offset, 1);

    /* upper-most bit clear = last byte */
    if ((byte & 0x80) == 0)
      break;
  }

  n_bytes = ret.size();
  return ret;
}

/* read in an SLEB128-encoded integer from the given location */
DwarfEncodedSLEB128
DwarfReadSLEB128FromSection(t_section *section, t_address offset, t_uint32& n_bytes)
{
  DwarfEncodedSLEB128 ret;

  /* only read up to the number of supported bytes */
  while (true)
  {
    /* read the next byte */
    t_uint8 byte = SectionGetData8(section, offset);
    ret.push_back(byte);
    offset = AddressAddUint32(offset, 1);

    /* upper-most bit clear = last byte */
    if ((byte & 0x80) == 0)
      break;
  }

  n_bytes = ret.size();
  return ret;
}

/* decode a given ULEB128-encoded integer */
DwarfDecodedULEB128 DwarfDecodeULEB128(DwarfEncodedULEB128 encoded)
{
  ASSERT(encoded.size() <= 10, ("Can only decode ULEB128 into a 64-bits unsigned integer at the most!"));

  DwarfDecodedULEB128 result = 0;
  int shift = 0;

  for (t_uint8 byte : encoded) {
    result |= (static_cast<t_int64>(byte) & 0x7f) << shift;
    shift += 7;
  }

  return result;
}

/* decode an SLEB128-encoded integer */
DwarfDecodedSLEB128
DwarfDecodeSLEB128(DwarfEncodedSLEB128 encoded)
{
  ASSERT(encoded.size() <= 10, ("Can only decode SLEB128 into a 64-bits integer at the most!"));

  DwarfDecodedSLEB128 result = 0;
  int shift = 0;
  int size = sizeof(DwarfDecodedSLEB128) * 8;

  t_uint8 byte;
  for (size_t i = 0; i < encoded.size(); i++) {
    byte = encoded[i];

    result |= (static_cast<t_int64>(byte) & 0x7f) << shift;
    shift += 7;
  }

  if ((shift < size)
      && (byte & 0x40)) {
    result |= -(1ULL << shift);
  }

  return result;
}

/* read in an initial length value.
 * Depending on whether or not a 64-bit address is used, additional data should be read. */
t_uint64
DwarfReadInitialLength(t_section *sec, t_address offset, t_uint32& size)
{
  t_uint64 ret = 0;
  t_uint32 length;

  /* read in the first 32-bit integer */
  length = SectionGetData32(sec, offset);
  size = 4;

  /* sanity check */
  ASSERT(length < 0xfffffff0, ("unsupported length field 0x%x", length));

  /* check for 64-bitness */
  if (length == 0xffffffff)
  {
    /* yes, a 64-bit length is specified */
    ret = SectionGetData64(sec, AddressAddUint32(offset, 4));
    size += 8;
  }
  else
    ret = static_cast<t_uint64>(length);

  return ret;
}

/* read in a NULL-terminated string from the given location */
std::string
ReadStringFromSection(t_section *section, t_address offset)
{
  /* a NULL-character is automatically appended */
  std::string ret = "";

  char kar;
  do
  {
    /* read in the next character */
    kar = static_cast<char>(SectionGetData8(section, offset));
    offset = AddressAddUint32(offset, 1);

    /* end-of-string reached */
    if (kar == '\0') break;

    /* not at end-of-string yet, append the character */
    ret.append(1, kar);
  } while (true);

  return ret;
}

/* read in an address from the given location */
t_address
DwarfReadAddress(t_uint8 address_size, t_section *section, t_address offset)
{
  t_address ret;

  switch(address_size) {
  case 4:
    ret = static_cast<t_address>(SectionGetData32(section, offset));
    break;

  case 8:
    ret = static_cast<t_address>(SectionGetData64(section, offset));
    break;

  default:
    FATAL(("unsupported address size %d", address_size));
  }

  return ret;
}
