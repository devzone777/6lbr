/*
 * Copyright (c) 2014, CETIC.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**
 * \file
 *         Simple CoAP Library
 * \author
 *         6LBR Team <6lbr@cetic.be>
 */
#include "coap-common.h"
#include "coap-data-format.h"

#define DEBUG 0
#include "net/ip/uip-debug.h"

unsigned long coap_batch_basetime = 0;

/*---------------------------------------------------------------------------*/
int
coap_strtoul(char const *data, char const *max, uint32_t *value) {
  *value = 0;
  while (data != max) {
    if(*data >= '0' && *data <= '9') {
      *value = (*value * 10) + (*data - '0');
    } else {
      return 0;
    }
    data++;
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
int
coap_strtofix(char const *data, char const *max, uint32_t *value, int precision) {
  int cur_precision = -1;
  *value = 0;
  while (data != max) {
    if(*data >= '0' && *data <= '9') {
      if(cur_precision < precision) {
        *value = (*value * 10) + (*data - '0');
        if(cur_precision != -1) {
          cur_precision++;
        }
      }
    } else if(*data == '.' && cur_precision == -1) {
      cur_precision = 0;
    } else {
      return 0;
    }
    data++;
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
int
coap_format_binding_value(int resource_type, char *buffer, int size, uint32_t data)
{
  switch(resource_type) {
  case COAP_RESOURCE_TYPE_SIGNED_INT:
    return snprintf(buffer, size, "%ld", (signed long int)data);
    break;
  case COAP_RESOURCE_TYPE_UNSIGNED_INT:
    return snprintf(buffer, size, "%ld", (unsigned long int)data);
    break;
  case COAP_RESOURCE_TYPE_DECIMAL_ONE:
    return snprintf(buffer, size, "%d.%u", (int)(data / 10), (unsigned int)(data % 10));
    break;
  case COAP_RESOURCE_TYPE_DECIMAL_TWO:
    return snprintf(buffer, size, "%d.%02u", (int)(data / 100), (unsigned int)(data % 100));
    break;
  case COAP_RESOURCE_TYPE_DECIMAL_THREE:
    return snprintf(buffer, size, "%d.%03u", (int)(data / 1000), (unsigned int)(data % 1000));
    break;
  default:
    break;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
int
coap_parse_binding_value(int resource_type, char const *buffer, char const * max, uint32_t *data)
{
  switch(resource_type) {
  case COAP_RESOURCE_TYPE_SIGNED_INT:
    return coap_strtoul(buffer, max, data);
    break;
  case COAP_RESOURCE_TYPE_UNSIGNED_INT:
    return coap_strtoul(buffer, max, data);
    break;
  case COAP_RESOURCE_TYPE_DECIMAL_ONE:
    return coap_strtofix(buffer, max, data, 1);
    break;
  case COAP_RESOURCE_TYPE_DECIMAL_TWO:
    return coap_strtofix(buffer, max, data, 2);
    break;
  case COAP_RESOURCE_TYPE_DECIMAL_THREE:
    return coap_strtofix(buffer, max, data, 3);
    break;
  default:
    break;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
resource_t*
rest_find_resource_by_url(const char *url)
{
  resource_t *resource;
  size_t len = strlen(url);
  for(resource = (resource_t *)list_head(rest_get_resources());
      resource; resource = resource->next) {
    if((len == strlen(resource->url)
        || (len > strlen(resource->url)
            && (resource->flags & HAS_SUB_RESOURCES)))
       && strncmp(resource->url, url, strlen(resource->url)) == 0) {
      return resource;
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
int
coap_add_ipaddr(char * buf, int size, const uip_ipaddr_t *addr)
{
  uint16_t a;
  unsigned int i;
  int f;
  int pos = 0;

  for(i = 0, f = 0; i < sizeof(uip_ipaddr_t); i += 2) {
    a = (addr->u8[i] << 8) + addr->u8[i + 1];
    if(a == 0 && f >= 0) {
      if(f++ == 0) {
        pos += snprintf(buf + pos, size - pos, "::");
      }
    } else {
      if(f > 0) {
        f = -1;
      } else if(i > 0) {
        pos += snprintf(buf + pos, size - pos, ":");
      }
      pos += snprintf(buf + pos, size - pos, "%x", a);
    }
  }
  return pos;
}
/*---------------------------------------------------------------------------*/
int
full_resource_config_attr_handler(coap_full_resource_t *resource_info, char *buffer, int size)
{
  int pos = 0;
  if((resource_info->trigger.flags & COAP_BINDING_FLAGS_PMIN_VALID) != 0) {
    pos += snprintf(buffer + pos, size - pos, ";pmin=\"%d\"", resource_info->trigger.pmin);
  }
  if((resource_info->trigger.flags & COAP_BINDING_FLAGS_PMAX_VALID) != 0) {
    pos += snprintf(buffer + pos, size - pos, ";pmax=\"%d\"", resource_info->trigger.pmax);
  }
  if((resource_info->trigger.flags & COAP_BINDING_FLAGS_ST_VALID) != 0) {
    pos += snprintf(buffer + pos, size - pos, ";st=\"");
    pos += coap_format_binding_value(resource_info->flags, buffer + pos, size - pos, resource_info->trigger.step);
    pos += snprintf(buffer + pos, size - pos, "\"");
  }
  if((resource_info->trigger.flags & COAP_BINDING_FLAGS_LT_VALID) != 0) {
    pos += snprintf(buffer + pos, size - pos, ";lt=\"");
    pos += coap_format_binding_value(resource_info->flags, buffer + pos, size - pos, resource_info->trigger.less_than);
    pos += snprintf(buffer + pos, size - pos, "\"");
  }
  if((resource_info->trigger.flags & COAP_BINDING_FLAGS_GT_VALID) != 0) {
    pos += snprintf(buffer + pos, size - pos, ";gt=\"");
    pos += coap_format_binding_value(resource_info->flags, buffer + pos, size - pos, resource_info->trigger.greater_than);
    pos += snprintf(buffer + pos, size - pos, "\"");
  }
  return pos;
}/*---------------------------------------------------------------------------*/
void
simple_resource_get_handler(int resource_type, char const *resource_name, uint32_t resource_value, void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  unsigned int accept = -1;
  if (request != NULL) {
    REST.get_header_accept(request, &accept);
  }
  if (coap_data_format.accepted_type(accept))
  {
    PRINTF("In Offset : %d, pref: %d\n", *offset, preferred_size);
    int buffer_size = coap_data_format.format_value((char *)buffer, preferred_size, *offset, accept, resource_type, resource_name, resource_value);
    REST.set_header_content_type(response, coap_data_format.format_type(accept));
    if(offset) {
      REST.set_response_payload(response, (uint8_t *)buffer, buffer_size);
      if(*offset + preferred_size >= buffer_size) {
        *offset = -1;
      } else {
        *offset += preferred_size;
      }
      PRINTF("Out Offset : %d, buf: %d\n", *offset, buffer_size);
    } else {
      REST.set_response_payload(response, (uint8_t *)buffer, preferred_size > buffer_size ? buffer_size : preferred_size);
    }
  } else {
    REST.set_response_status(response, REST.status.NOT_ACCEPTABLE);
  }
}
/*---------------------------------------------------------------------------*/
void
simple_resource_set_handler(int resource_type, char const * resource_name, int(*resource_set)(uint32_t value, uint32_t len), void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  unsigned int accept = -1;
  if (request != NULL) {
    REST.get_header_accept(request, &accept);
  }
  if (coap_data_format.accepted_type(accept))
  {
    const uint8_t * payload;
    size_t len = REST.get_request_payload(request, &payload);
    uint32_t value;
    if (coap_data_format.parse_value((char *)payload, (char *)(payload + len), accept, resource_type, resource_name, &value)) {
      if(resource_set(value, len)) {
        REST.set_response_status(response, REST.status.CHANGED);
      } else {
        REST.set_response_status(response, REST.status.BAD_REQUEST);
      }
    } else {
      REST.set_response_status(response, REST.status.BAD_REQUEST);
    }
  } else {
    REST.set_response_status(response, REST.status.NOT_ACCEPTABLE);
  }
}
/*---------------------------------------------------------------------------*/
void
full_resource_get_handler(coap_full_resource_t *resource_info, void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  unsigned int accept = -1;
  if (request != NULL) {
    REST.get_header_accept(request, &accept);
  }
  if (coap_data_format.accepted_type(accept))
  {
    int buffer_size = coap_data_format.format_value((char *)buffer, preferred_size, *offset, accept, resource_info->flags, resource_info->name, resource_info->data.last_value);
    REST.set_header_content_type(response, coap_data_format.format_type(accept));
    if(offset) {
      REST.set_response_payload(response, (uint8_t *)buffer, buffer_size);
      if(*offset + preferred_size >= buffer_size) {
        *offset = -1;
      } else {
        *offset += preferred_size;
      }
      PRINTF("Offset : %d\n", *offset);
    } else {
      REST.set_response_payload(response, (uint8_t *)buffer, preferred_size > buffer_size ? buffer_size : preferred_size);
    }
  } else if (accept == APPLICATION_LINK_FORMAT) {
    REST.set_header_content_type(response, APPLICATION_LINK_FORMAT);
    int pos = 0;
    pos += snprintf((char *)buffer + pos, preferred_size - pos, "</%s>", resource_info->coap_resource->url);
    pos += full_resource_config_attr_handler(resource_info, (char *)buffer + pos, preferred_size - pos);
    int buffer_size = pos;
    if(offset) {
      REST.set_response_payload(response, (uint8_t *)buffer + *offset, *offset + preferred_size > buffer_size ? buffer_size - *offset : preferred_size);
      if(*offset + preferred_size >= buffer_size) {
        *offset = -1;
      } else {
        *offset += preferred_size;
      }
      PRINTF("Offset : %d\n", *offset);
    } else {
      REST.set_response_payload(response, (uint8_t *)buffer, preferred_size > buffer_size ? buffer_size : preferred_size);
    }
  } else {
    REST.set_response_status(response, REST.status.NOT_ACCEPTABLE);
  }
}
/*---------------------------------------------------------------------------*/
void
full_resource_config_handler(coap_full_resource_t *resource_info, void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) \
{
  unsigned int accept = -1;
  if (request != NULL) {
    REST.get_header_accept(request, &accept);
  }
  if (accept == -1 || accept == APPLICATION_LINK_FORMAT)
  {
    const uint8_t * payload;
    int success = 0;
    size_t len = REST.get_request_payload(request, &payload);
    if(len == 0) {
      const char * query;
      len = coap_get_header_uri_query(request, &query);
      success = coap_binding_parse_filters((char *)query, len, &resource_info->trigger, resource_info->flags);
    }
    REST.set_response_status(response, success ? REST.status.CHANGED : REST.status.BAD_REQUEST);
  } else {
    REST.set_response_status(response, REST.status.NOT_ACCEPTABLE);
  }
}
/*---------------------------------------------------------------------------*/