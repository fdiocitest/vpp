/*
 * Copyright (c) 2017 Cisco and/or its affiliates.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "vom/acl_l2_list.hpp"
#include "vom/acl_list_cmds.hpp"
#include "vom/logger.hpp"
#include "vom/singular_db_funcs.hpp"

namespace VOM {
namespace ACL {

/**
 * Definition of the static singular_db for ACL Lists
 */
singular_db<l2_list::key_t, l2_list> l2_list::m_db;

/**
 * Definition of the static per-handle DB for ACL Lists
 */
std::map<handle_t, std::weak_ptr<l2_list>> l2_list::m_hdl_db;

l2_list::event_handler l2_list::m_evh;

l2_list::event_handler::event_handler()
{
  OM::register_listener(this);
  inspect::register_handler({ "l2-acl-list" }, "L2 ACL lists", this);
}

l2_list::l2_list(const key_t& key)
  : m_hdl(handle_t::INVALID)
  , m_key(key)
{
}

l2_list::l2_list(const handle_t& hdl, const key_t& key)
  : m_hdl(hdl)
  , m_key(key)
{
}

l2_list::l2_list(const key_t& key, const rules_t& rules)
  : m_hdl(handle_t::INVALID)
  , m_key(key)
  , m_rules(rules)
{
}

l2_list::l2_list(const l2_list& o)
  : m_hdl(o.m_hdl)
  , m_key(o.m_key)
  , m_rules(o.m_rules)
{
}

l2_list::~l2_list()
{
  sweep();
  m_db.release(m_key, this);
}

std::shared_ptr<l2_list>
l2_list::singular() const
{
  return find_or_add(*this);
}

/**
 * Dump all ACLs into the stream provided
 */
void
l2_list::dump(std::ostream& os)
{
  db_dump(m_db, os);
}

/**
 * convert to string format for debug purposes
 */
std::string
l2_list::to_string() const
{
  std::ostringstream s;
  s << "acl-list:[" << m_key << " " << m_hdl.to_string() << " rules:[";

  for (auto rule : m_rules) {
    s << rule.to_string() << " ";
  }

  s << "]]";

  return (s.str());
}

void
l2_list::insert(const l2_rule& rule)
{
  m_rules.insert(rule);
}

void
l2_list::remove(const l2_rule& rule)
{
  m_rules.erase(rule);
}

const handle_t&
l2_list::handle() const
{
  return (singular()->handle_i());
}

std::shared_ptr<l2_list>
l2_list::find(const handle_t& handle)
{
  return (m_hdl_db[handle].lock());
}

std::shared_ptr<l2_list>
l2_list::find(const key_t& key)
{
  return (m_db.find(key));
}

std::shared_ptr<l2_list>
l2_list::find_or_add(const l2_list& temp)
{
  return (m_db.find_or_add(temp.key(), temp));
}

const handle_t&
l2_list::handle_i() const
{
  return (m_hdl.data());
}

void
l2_list::add(const key_t& key, const HW::item<handle_t>& item)
{
  std::shared_ptr<l2_list> sp = find(key);

  if (sp && item) {
    m_hdl_db[item.data()] = sp;
  }
}

void
l2_list::remove(const HW::item<handle_t>& item)
{
  m_hdl_db.erase(item.data());
}

const l2_list::key_t&
l2_list::key() const
{
  return m_key;
}

const l2_list::rules_t&
l2_list::rules() const
{
  return m_rules;
}

bool
l2_list::operator==(const l2_list& l) const
{
  return (key() == l.key() && rules() == l.rules());
}

void
l2_list::event_handler::handle_populate(const client_db::key_t& key)
{
  /*
   * dump L2 ACLs
   */
  std::shared_ptr<list_cmds::l2_dump_cmd> cmd =
    std::make_shared<list_cmds::l2_dump_cmd>();

  HW::enqueue(cmd);
  HW::write();

  for (auto& record : *cmd) {
    auto& payload = record.get_payload();

    const handle_t hdl(payload.acl_index);
    l2_list acl(hdl, std::string(reinterpret_cast<const char*>(payload.tag)));

    for (unsigned int ii = 0; ii < payload.count; ii++) {
      const route::prefix_t pfx(payload.r[ii].is_ipv6,
                                payload.r[ii].src_ip_addr,
                                payload.r[ii].src_ip_prefix_len);
      l2_rule rule(ii, action_t::from_int(payload.r[ii].is_permit), pfx,
                   { payload.r[ii].src_mac }, { payload.r[ii].src_mac_mask });

      acl.insert(rule);
    }
    VOM_LOG(log_level_t::DEBUG) << "dump: " << acl.to_string();

    /*
     * Write each of the discovered ACLs into the OM,
     * but disable the HW Command q whilst we do, so that no
     * commands are sent to VPP
     */
    OM::commit(key, acl);
  }
}

void
l2_list::event_handler::show(std::ostream& os)
{
  db_dump(m_db, os);
}

dependency_t
l2_list::event_handler::order() const
{
  return (dependency_t::ACL);
}

void
l2_list::event_handler::handle_replay()
{
  m_db.replay();
}

void
l2_list::update(const l2_list& obj)
{
  /*
   * always update the instance with the latest rule set
   */
  if (rc_t::OK != m_hdl.rc() || obj.m_rules != m_rules) {
    HW::enqueue(new list_cmds::l2_update_cmd(m_hdl, m_key, m_rules));
  }
  /*
   * We don't, can't, read the priority from VPP,
   * so the is equals check above does not include the priorty.
   * but we save it now.
   */
  m_rules = obj.m_rules;
}

void
l2_list::sweep(void)
{
  if (m_hdl) {
    HW::enqueue(new list_cmds::l2_delete_cmd(m_hdl));
  }
  HW::write();
}

void
l2_list::replay(void)
{
  if (m_hdl) {
    m_hdl.data().reset();
    HW::enqueue(new list_cmds::l2_update_cmd(m_hdl, m_key, m_rules));
  }
}

}; // namespace ACL
}; // namespace VOM

/*
 * fd.io coding-style-patch-verification: ON
 *
 * Local Variables:
 * eval: (c-set-style "mozilla")
 * End:
 */
