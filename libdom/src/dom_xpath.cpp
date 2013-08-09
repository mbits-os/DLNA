/*
 * Copyright (C) 2013 midnightBITS
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <dom.hpp>
#include <dom_xpath.hpp>
#include <vector>
#include <string.h>

namespace dom {
	XmlNodeListPtr createList(const std::vector<XmlNodePtr>& list);
};

namespace dom { namespace xpath {
	XPath::XPath(const std::string& xpath, const Namespaces& ns)
	{
		const char* ptr = xpath.c_str();
		if (!ptr || !*ptr)
			return;
		const char* end = ptr + xpath.length();
		if (ptr < end && *ptr == '/')
		{
			Segment seg;
			seg.m_selector.m_axis = AXIS_SELF;
			seg.m_selector.m_test = TEST_DOCUMENT_NODE;
			m_segments.push_back(seg);
			++ptr;
		}

		while (ptr && ptr != end)
		{
			Segment seg;
			ptr = readSegment(ptr, end, seg, ns);
			if (ptr)
			{
				m_segments.push_back(seg);
				if (ptr < end) ++ptr; //skip the slash
			}
		}
	}

	class SelectorAction
	{
	public:
		SelectorAction()
		{
		}
	};

	static bool like(const QName& name, const QName& tmplt)
	{
		if (tmplt.nsName.empty() && tmplt.localName.empty())
			return true;
		if (tmplt.nsName != "*" && tmplt.nsName != name.nsName)
			return false;
		if (tmplt.localName != "*" && tmplt.localName != name.localName)
			return false;
		return true;
	}

	bool SimpleSelector::passable(XmlNodePtr& node)
	{
		switch(m_test)
		{
		case TEST_NODE: return true;
		case TEST_TEXT: return node->nodeType() == TEXT_NODE;
		case TEST_DOCUMENT_NODE:
			node = node->ownerDocument();
			return true;
		case TEST_ELEMENT:
			if (node->nodeType() != ELEMENT_NODE)
				return false;
			return like(node->nodeQName(), m_name);
		case TEST_ATTRIBUTE:
			if (node->nodeType() != ATTRIBUTE_NODE)
				return false;
			return like(node->nodeQName(), m_name);
		}
		return false;
	}

	void SimpleSelector::test(const XmlNodePtr& node, std::list<XmlNodePtr>& list)
	{
		auto pass = node;
		if (passable(pass))
			list.push_back(pass);
	}
	
	void SimpleSelector::select(const XmlNodeListPtr& nodes, std::list<XmlNodePtr>& list)
	{
		size_t count = nodes ? nodes->length() : 0;
		for (size_t i = 0; i < count; ++i)
			test(nodes->item(i), list);
	}

	void SimpleSelector::select(const XmlNodePtr& context, std::list<XmlNodePtr>& list)
	{
		switch(m_axis)
		{
		case AXIS_CHILD:
			attribute(context, list);
			child(context, list);
			return;
		case AXIS_DESCENDANT:
			descendant(context, list);
			return;
		case AXIS_ATTRIBUTE:
			attribute(context, list);
			return;
		case AXIS_SELF:
			self(context, list);
			return;
		case AXIS_DESCENDANT_OR_SELF:
			descendant_or_self(context, list);
			return;
		case AXIS_PARENT:
			parent(context, list);
			return;
		case AXIS_ANCESTOR:
			ancestor(context, list);
			return;
		case AXIS_ANCESTOR_OR_SELF:
			ancestor_or_self(context, list);
			return;
		}
	}

	void SimpleSelector::child(const XmlNodePtr& context, std::list<XmlNodePtr>& list)
	{
		if (!context) return;
		select(context->childNodes(), list);
	}

	void SimpleSelector::descendant(const XmlNodePtr& context, std::list<XmlNodePtr>& list)
	{
		if (!context) return;
		auto nodes = context->childNodes();
		select(nodes, list);

		size_t count = nodes ? nodes->length() : 0;
		for (size_t i = 0; i < count; ++i)
			descendant(nodes->item(i), list);
	}

	void SimpleSelector::attribute(const XmlNodePtr& context, std::list<XmlNodePtr>& list)
	{
		if (!context) return;
		if (context->nodeType() == ELEMENT_NODE)
		{
			auto elem = std::static_pointer_cast<XmlElement>(context);
			select(elem->getAttributes(), list);
		}
	}

	void SimpleSelector::self(const XmlNodePtr& context, std::list<XmlNodePtr>& list)
	{
		if (!context) return;
		test(context, list);
	}

	void SimpleSelector::descendant_or_self(const XmlNodePtr& context, std::list<XmlNodePtr>& list)
	{
		self(context, list);
		descendant(context, list);
	}

	void SimpleSelector::parent(const XmlNodePtr& context, std::list<XmlNodePtr>& list)
	{
		if (!context) return;
		XmlNodePtr parent = context->parentNode();
		if (!parent) return;
		test(parent, list);
	}

	void SimpleSelector::ancestor(const XmlNodePtr& context, std::list<XmlNodePtr>& list)
	{
		if (!context) return;
		XmlNodePtr parent = context->parentNode();
		while (parent)
		{
			test(parent, list);
			parent = parent->parentNode();
		}
	}

	void SimpleSelector::ancestor_or_self(const XmlNodePtr& context, std::list<XmlNodePtr>& list)
	{
		self(context, list);
		ancestor(context, list);
	}

	template <typename Container>
	std::list<XmlNodePtr> select(Container& container, const XmlNodePtr& context)
	{
		std::list<XmlNodePtr> parent;
		parent.push_back(context);

		for (auto && query : container)
		{
			std::list<XmlNodePtr> list;
			for (auto && ctx : parent)
				query.select(ctx, list);
			parent = list;
		}

		return parent;
	}

	bool Predicate::test(const XmlNodePtr& context)
	{
		std::list<XmlNodePtr> list = select(m_selectors, context);

		if (m_type == PRED_EXISTS)
			return !list.empty();

		for (auto && node : list)
		{
			if (node && node->stringValue() == m_value)
				return true;
		}
		return false;
	}

	void Segment::select(const XmlNodePtr& context, std::list<XmlNodePtr>& list)
	{
		std::list<XmlNodePtr> local;
		m_selector.select(context, local);
		for (auto && node : local)
		{
			bool failed = false;
			for (auto && pred : m_preds)
			{
				if (!pred.test(node))
				{
					failed = true;
					break;
				}
			}
			if (!failed)
				list.push_back(node);
		};
	}

	XmlNodePtr XPath::find(const XmlNodePtr& context)
	{
		std::list<XmlNodePtr> list = select(m_segments, context);
		if (list.size())
			return *list.begin();
		return nullptr;
	}

	XmlNodeListPtr XPath::findall(const XmlNodePtr& context)
	{
		std::list<XmlNodePtr> list = select(m_segments, context);
		if (list.size())
		{
			std::vector<XmlNodePtr> nodes(list.begin(), list.end());
			return createList(nodes);
		}
		return nullptr;
	}

	static const char* axis_names[] = {
		"child",
		"descendant",
		"attribute",
		"self",
		"descendant-or-self",
		"parent",
		"ancestor",
		"ancestor-or-self"
	};

	static const char* test_names[] = {
		"node",
		"text",
		"element",
		"attribute",
		"document-node"
	};

	inline const char* FindNamespace(const Namespaces& ns, const char* key)
	{
		if (!ns)
			return nullptr;

		NSData* item = ns;
		while (item->key)
		{
			if (strcmp(item->key, key) == 0) return item->ns;
			item++;
		}
		return nullptr;
	}

	static inline const char* readQName(const char* ptr, const char* end, QName& qname, const Namespaces& ns)
	{
		if (ptr + 1 == end && ptr[0] == '*')
		{
			qname.nsName = "*";
			qname.localName = "*";
			return end;
		}
		if (ptr + 1 < end && ptr[0] == '*' && ptr[1] == ':')
		{
			qname.nsName = "*";
			qname.localName.assign(ptr + 2, end);
			return end;
		}

		qname.localName.assign(ptr, end);
		std::string::size_type col = qname.localName.find(':');

		if (col != std::string::npos)
		{
			const char* _it = FindNamespace(ns, std::string(ptr, col).c_str());
			if (_it != nullptr)
			{
				qname.nsName = _it;
				qname.localName = std::string(ptr + col + 1, end);
			}
		}

		return end;
	}

	static inline const char* readSelector(const char* ptr, const char* end, SimpleSelector& sel, const Namespaces& ns)
	{
		//.. is shorthand for parent::node()
		if (ptr + 1 < end && ptr[0] =='.' && ptr[1] == '.')
		{
			sel.m_axis = AXIS_PARENT;
			sel.m_test = TEST_NODE;
			return ptr + 2;
		}

		// If the axis name is omitted from an axis step, the default axis
		// is 'child' unless the axis step contains an AttributeTest[..];
		// in that case, the default axis is 'attribute'.
		bool child_by_default = true;

		//look for an axis:
		const char* save = ptr;
		while (ptr < end && (*ptr >= 'a' && *ptr <= 'z' || *ptr == '-')) ++ptr;

		if (ptr + 1 < end && ptr[0] == ':' && ptr[1] == ':')
		{
			size_t count = ptr - save;
			size_t i = 0;
			for (; i < sizeof(axis_names)/sizeof(axis_names[0]); ++i)
			{
				if (!strncmp(save, axis_names[i], count) &&
					strlen(axis_names[i]) == count)
				{
					sel.m_axis = (AXIS)i;
					break;
				}
			}
			if (i == sizeof(axis_names)/sizeof(axis_names[0]))
				return nullptr;
			ptr += 2;
			save = ptr;
			child_by_default = false;

			//look for test function:
			while (ptr < end && (*ptr >= 'a' && *ptr <= 'z' || *ptr == '-')) ++ptr;
		}
		// else try test function

		if (ptr < end && *ptr == '(')
		{
			size_t count = ptr - save;
			size_t i = 0;
			for (; i < sizeof(test_names)/sizeof(test_names[0]); ++i)
			{
				if (!strncmp(save, test_names[i], count) &&
					strlen(test_names[i]) == count)
				{
					sel.m_test = (TEST)i;
					break;
				}
			}
			if (i == sizeof(test_names)/sizeof(test_names[0]))
				return nullptr;

			ptr++;
			save = ptr;

			//optional name
			if (sel.m_test == TEST_ELEMENT || sel.m_test == TEST_ATTRIBUTE)
			{
				while (ptr < end && *ptr != ')') ++ptr;
				if (ptr > save)
				{
					const char* ret = readQName(save, ptr, sel.m_name, ns);
					if (ret != ptr) // syntax error
						return nullptr;
				}
			}
			if (ptr >= end || *ptr != ')')
				return nullptr;

			if (child_by_default && sel.m_test == TEST_ATTRIBUTE)
				sel.m_axis = AXIS_ATTRIBUTE;

			return ptr + 1;
		}

		//move past the name test
		while (ptr < end && *ptr != '/' && *ptr != '[' && *ptr != '=') ++ptr;
		if (save < ptr && *save == '@')
		{
			++save;
			sel.m_test = TEST_ATTRIBUTE;
			if (child_by_default)
				sel.m_axis = AXIS_ATTRIBUTE;
		}
		else
			sel.m_test = TEST_ELEMENT;

		const char* ret = readQName(save, ptr, sel.m_name, ns);
		if (ret != ptr) // syntax error
			return nullptr;

		if (sel.m_axis == AXIS_ATTRIBUTE)
			sel.m_test = TEST_ATTRIBUTE; //to allow attribute::href to mean @href

		return ptr;
	}

	static inline const char* readPredicate(const char* ptr, const char* end, Predicate& pred, const Namespaces& ns)
	{
		++ptr;

		while (ptr < end && isspace((unsigned char)*ptr)) ++ptr;

		SimpleSelector selector;
		ptr = readSelector(ptr, end, selector, ns);
		if (!ptr)
			return nullptr;

		pred.m_selectors.push_back(selector);
		while(ptr < end && *ptr == '/')
		{
			++ptr;
			SimpleSelector selector;
			ptr = readSelector(ptr, end, selector, ns);
			if (!ptr)
				return nullptr;

			pred.m_selectors.push_back(selector);
		}

		while (ptr < end && isspace((unsigned char)*ptr)) ++ptr;
		if (ptr < end && *ptr == '=')
		{
			pred.m_type = PRED_EQUALS;
			++ptr;
			while (ptr < end && isspace((unsigned char)*ptr)) ++ptr;
			const char* value = ptr;
			if (ptr < end && *ptr == '\'')
			{
				++value;
				++ptr;
				while (ptr < end && *ptr != '\'') ++ptr;
				pred.m_value.assign(value, ptr);
				++ptr;
			}
			else if (ptr < end && *ptr == '"')
			{
				++value;
				++ptr;
				while (ptr < end && *ptr != '"') ++ptr;
				pred.m_value.assign(value, ptr);
				++ptr;
			}
			else
			{
				while (ptr < end && (isdigit((unsigned char)*ptr) || *ptr == '.' || *ptr == 'e' || *ptr == 'E' || *ptr == '+' || *ptr == '-')) ++ptr;
				pred.m_value.assign(value, ptr);
			}
		}
		while (ptr < end && isspace((unsigned char)*ptr)) ++ptr;
		if (ptr < end && *ptr != ']')
			return nullptr;
		return ptr + 1;
	}

	const char* XPath::readSegment(const char* ptr, const char* end, Segment& seg, const Namespaces& ns)
	{
		if (ptr < end && *ptr == '/') // it was '//' - a shorthand for descendant-or-self::node()
		{
			seg.m_selector.m_axis = AXIS_DESCENDANT_OR_SELF;
			seg.m_selector.m_test = TEST_NODE;
			return ptr; //will be incremented by the caller
		}
		ptr = readSelector(ptr, end, seg.m_selector, ns);
		if (!ptr)
			return nullptr;
		while (ptr && ptr < end && *ptr == '[')
		{
			Predicate pred;
			ptr = readPredicate(ptr, end, pred, ns);
			if (ptr)
				seg.m_preds.push_back(pred);
		}
		if (ptr && ptr < end && *ptr != '/')
			return nullptr;

		return ptr;
	}

	std::ostream& operator << (std::ostream& o, const SimpleSelector& sel)
	{
		o << axis_names[sel.m_axis] << "::" << test_names[sel.m_test] << "(";
		if (!sel.m_name.nsName.empty())
			o << "{" << sel.m_name.nsName << "}";
		o << sel.m_name.localName;
		o << ")";
		return o;
	}
	std::ostream& operator << (std::ostream& o, const Predicate& pred)
	{
		o << "[";
		bool first = true;
		for (auto && selector : pred.m_selectors)
		{
			if (first) first = false;
			else o << "/";
			o << selector;
		};
		if (pred.m_type == PRED_EQUALS)
			o << "='" << pred.m_value << "'";
		return o << "]";
	}
	std::ostream& operator << (std::ostream& o, const Segment& seg)
	{
		o << seg.m_selector;
		for (auto && pred : seg.m_preds)
			o << pred;
		return o;
	}

	std::ostream& operator << (std::ostream& o, const XPath& xpath)
	{
		bool first = true;
		for (auto && seg : xpath.m_segments)
		{
			if (first) first = false;
			else o << "/";
			o << seg;
		};

		return o;
	}
}}