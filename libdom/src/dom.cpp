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
#include <map>
#include <iterator>
#include <string.h>
#include <expat.hpp>
#ifdef WIN32
#include <windows.h>
#endif

namespace dom
{
	namespace impl
	{
		typedef std::vector< dom::XmlNodePtr > NodesInit;

		struct XmlNodeImplInit
		{
			NODE_TYPE type;
			std::string _name, _value;
			std::vector<dom::XmlNodePtr> children;
			std::weak_ptr<dom::XmlDocument> document;
			std::weak_ptr<dom::XmlNode> parent;
			std::weak_ptr<dom::XmlNode> self;
			size_t index;
			QName qname;

			virtual void fixQName(bool forElem = true)
			{
				std::string::size_type col = _name.find(':');
				if (col == std::string::npos && !forElem) return;
				if (col == std::string::npos)
					fixQName(qname, std::string(), _name);
				else
					fixQName(qname, std::string(_name.c_str(), col), std::string(_name.c_str() + col + 1));
			}

			virtual void fixQName(QName& qname, const std::string& ns, const std::string& localName)
			{
				dom::XmlNodePtr parent = this->parent.lock();
				if (!parent) return;
				XmlNodeImplInit* p = (XmlNodeImplInit*)parent->internalData();
				if (!p) return;
				p->fixQName(qname, ns, localName);
			}
		};

		template <typename T, typename _Interface>
		class XmlNodeImpl: public _Interface, public XmlNodeImplInit
		{
		public:

			typedef XmlNodeImplInit Init;
			typedef _Interface Interface;

			XmlNodeImpl(const Init& init): Init(init)
			{
				qname.localName = init._name;
			}

			std::string nodeName() const { return _name; }
			const QName& nodeQName() const { return qname; }
			std::string nodeValue() const { return _value; }
			void nodeValue(const std::string& val)
			{
				if (type != ELEMENT_NODE)
					_value = val;
			}

			NODE_TYPE nodeType() const { return type; }

			dom::XmlNodePtr parentNode() { return parent.lock(); }
			inline dom::XmlNodeListPtr childNodes();
			dom::XmlNodePtr firstChild()
			{
				if (!children.size()) return dom::XmlNodePtr();
				return children[0];
			}

			dom::XmlNodePtr lastChild()
			{
				if (!children.size()) return dom::XmlNodePtr();
				return children[children.size() - 1];
			}

			dom::XmlNodePtr previousSibling()
			{
				dom::XmlNodePtr par = parent.lock();
				if (!index || !par.get())
					return dom::XmlNodePtr();

				XmlNodeImplInit* plist = (XmlNodeImplInit*)par->internalData();
				if (!plist) return nullptr;
				return plist->children[index - 1];
			}

			dom::XmlNodePtr nextSibling()
			{
				dom::XmlNodePtr par = parent.lock();
				if (!par.get())
					return dom::XmlNodePtr();

				XmlNodeImplInit* plist = (XmlNodeImplInit*)par->internalData();
				if (!plist) return nullptr;

				size_t ndx = index + 1;
				if (ndx >= plist->children.size()) return dom::XmlNodePtr();

				return plist->children[ndx];
			}

			dom::XmlDocumentPtr ownerDocument() { return document.lock(); }
			bool appendChild(dom::XmlNodePtr newChild)
			{
				if (!newChild) return false;
				dom::XmlDocumentPtr doc = newChild->ownerDocument();
				if (!doc.get() || doc.get() != document.lock().get()) return false;

				if (newChild->nodeType() == ATTRIBUTE_NODE)
					return ((T*)this)->appendAttr(newChild);

				XmlNodeImplInit* p = (XmlNodeImplInit*)newChild->internalData();
				p->parent = self;
				p->index = children.size();
				children.push_back(newChild);
				p->fixQName();

				return true;
			}

			void* internalData() { return (XmlNodeImplInit*)this; }

			bool appendAttr(dom::XmlNodePtr newChild) { return false; }

			XmlNodePtr find(const std::string& path, const Namespaces& ns)
			{
				return xpath::XPath(path, ns).find(self.lock());
			}
			XmlNodeListPtr findall(const std::string& path, const Namespaces& ns)
			{
				return xpath::XPath(path, ns).findall(self.lock());
			}
		};

		class XmlNodeList: public dom::XmlNodeList
		{
		public:
			typedef NodesInit Init;
			Init children;

			XmlNodeList(const Init& init): children(init) {}
			~XmlNodeList() {}

			dom::XmlNodePtr item(size_t index)
			{
				if (index >= children.size()) return dom::XmlNodePtr();
				return children[index];
			}

			size_t length() const
			{
				return children.size();
			}
		};

		template <typename T, typename _Interface>
		dom::XmlNodeListPtr XmlNodeImpl<T, _Interface>::childNodes()
		{
			try {
			return std::make_shared<XmlNodeList>(children);
			} catch(std::bad_alloc) { return nullptr; }
		}

		class XmlAttribute: public XmlNodeImpl<XmlAttribute, dom::XmlAttribute>
		{
		public:
			XmlAttribute(const Init& init): XmlNodeImpl(init) {}

			dom::XmlNodePtr previousSibling()
			{
				return false;
			}

			dom::XmlNodePtr nextSibling()
			{
				return false;
			}
		};

		class XmlText: public XmlNodeImpl<XmlText, dom::XmlText>
		{
		public:
			XmlText(const Init& init): XmlNodeImpl(init) {}
		};

		class XmlElement: public XmlNodeImpl<XmlElement, dom::XmlElement>
		{
		public:
			bool nsRebuilt;
			typedef std::map< std::string, std::string > InternalNamespaces;
			InternalNamespaces namespaces;

			std::map< std::string, dom::XmlAttributePtr > lookup;

			XmlElement(const Init& init): XmlNodeImpl(init), nsRebuilt(false) {}

			std::string getAttribute(const std::string& name)
			{
				std::map< std::string, dom::XmlAttributePtr >::const_iterator
					_it = lookup.find(name);
				if (_it == lookup.end()) return std::string();
				return _it->second->value();
			}

			dom::XmlAttributePtr getAttributeNode(const std::string& name)
			{
				std::map< std::string, dom::XmlAttributePtr >::const_iterator
					_it = lookup.find(name);
				if (_it == lookup.end()) return dom::XmlAttributePtr();
				return _it->second;
			}

			bool setAttribute(dom::XmlAttributePtr attr)
			{
				XmlNodeImplInit* p = (XmlNodeImplInit*)attr->internalData();
				if (p)
					p->parent = self;

				std::map< std::string, dom::XmlAttributePtr >::const_iterator
					_it = lookup.find(attr->name());
				if (_it != lookup.end())
				{
					_it->second->value(attr->value());
					return true;
				}
				lookup[attr->name()] = attr;
				return true;
			}

			dom::XmlNodeListPtr getAttributes()
			{
				std::vector< dom::XmlNodePtr > out;
				for (auto && pair : lookup)
				{
					out.push_back(pair.second);
				}
				return std::make_shared<XmlNodeList>(out);
			}

			bool hasAttribute(const std::string& name)
			{
				std::map< std::string, dom::XmlAttributePtr >::const_iterator
					_it = lookup.find(name);
				return _it != lookup.end();
			}

			void enumTagNames(const std::string& tagName, std::vector< dom::XmlNodePtr >& out)
			{
				if (tagName == _name) out.push_back(self.lock());
				std::vector< dom::XmlNodePtr >::const_iterator
					_cur = children.begin(), _end = children.end();

				for (; _cur != _end; ++_cur)
				{
					const dom::XmlNodePtr& node = *_cur;
					if (node->nodeType() == ELEMENT_NODE)
						((XmlElement*)node.get())->enumTagNames(tagName, out);
				}
			}

			dom::XmlNodeListPtr getElementsByTagName(const std::string& tagName)
			{
				std::vector< dom::XmlNodePtr > out;
				enumTagNames(tagName, out);
				return std::make_shared<XmlNodeList>(out);
			}

			bool appendAttr(dom::XmlNodePtr newChild)
			{
				if (!newChild.get() || newChild->nodeType() != dom::ATTRIBUTE_NODE)
					return false;
				return setAttribute(std::static_pointer_cast<dom::XmlAttribute>(newChild));
			}

			std::string innerText()
			{
				//special case:
				if (children.size() == 1 && children[0] && children[0]->nodeType() == TEXT_NODE)
					return children[0]->nodeValue();

				std::string out;

				std::vector< dom::XmlNodePtr >::const_iterator
					_cur = children.begin(), _end = children.end();

				for (; _cur != _end; ++_cur)
				{
					auto node = *_cur;
					if (node.get())
						switch(node->nodeType())
						{
						case TEXT_NODE: out += node->nodeValue(); break;
						case ELEMENT_NODE:
							out += std::static_pointer_cast<dom::XmlElement>(node)->innerText();
							break;
						};
				}

				return out;
			}

			void fixQName(bool forElem = true)
			{
				XmlNodeImpl<XmlElement, dom::XmlElement>::fixQName(forElem);
				if (!forElem) return;
				for (auto && pair : lookup)
				{
					if (strncmp(pair.first.c_str(), "xmlns", 5) == 0 &&
						(pair.first.length() == 5 || pair.first[5] == ':'))
					{
						return;
					}
					XmlNodeImplInit* p = (XmlNodeImplInit*)pair.second->internalData();
					if (!p) return;
					p->fixQName(false);
				};
			}

			void fixQName(QName& qname, const std::string& ns, const std::string& localName)
			{
				if (!nsRebuilt)
				{
					nsRebuilt = true;
					namespaces.clear();
					for (auto && pair : lookup)
					{
						if (strncmp(pair.first.c_str(), "xmlns", 5) != 0) return;
						if (pair.first.length() != 5 && pair.first[5] != ':') return;
						if (pair.first.length() == 5) namespaces[""] = pair.second->value();
						else namespaces[std::string(pair.first.c_str() + 6)] = pair.second->value();
					};
				}
				InternalNamespaces::const_iterator _it = namespaces.find(ns);
				if (_it != namespaces.end())
				{
					qname.nsName = _it->second;
					qname.localName = localName;
					return;
				}
				XmlNodeImpl<XmlElement, dom::XmlElement>::fixQName(qname, ns, localName);
			}
		};

		class XmlDocument: public dom::XmlDocument
		{
			QName m_qname;
			dom::XmlElementPtr root;
			std::weak_ptr<dom::XmlDocument> self;

			friend XmlDocumentPtr dom::XmlDocument::create();
		public:

			XmlDocument()
			{
				m_qname.localName = "#document";
			}
			~XmlDocument() {}

			std::string nodeName() const { return m_qname.localName; }
			const QName& nodeQName() const { return m_qname; }
			std::string nodeValue() const { return std::string(); }
			void nodeValue(const std::string&) {}
			NODE_TYPE nodeType() const { return DOCUMENT_NODE; }

			XmlNodePtr parentNode() { return nullptr; }
			XmlNodePtr firstChild() { return documentElement(); }
			XmlNodePtr lastChild() { return documentElement(); }
			XmlNodePtr previousSibling() { return nullptr; }
			XmlNodePtr nextSibling() { return nullptr; }
			XmlNodeListPtr childNodes()
			{
				if (!root)
					return nullptr;
				try {
				NodesInit children;
				children.push_back(root);
				return std::make_shared<XmlNodeList>(children);
				} catch(std::bad_alloc) { return nullptr; }
			}

			XmlDocumentPtr ownerDocument() { return self.lock(); }
			bool appendChild(XmlNodePtr newChild) { return false; }
			void* internalData() { return nullptr; }

			dom::XmlElementPtr documentElement() { return root; }
			void setDocumentElement(dom::XmlElementPtr elem)
			{
				root = elem;
				if (elem)
					((XmlNodeImplInit*)elem->internalData())->fixQName();
			}
			dom::XmlElementPtr createElement(const std::string& tagName)
			{
				XmlNodeImplInit init;
				init.type = ELEMENT_NODE;
				init._name = tagName;
				//init._value;
				init.document = self;
				init.index = 0;
				auto out = std::make_shared<XmlElement>(init);
				((XmlNodeImplInit*)out->internalData())->self = out;
				return out;
			}

			dom::XmlTextPtr createTextNode(const std::string& data)
			{
				XmlNodeImplInit init;
				init.type = TEXT_NODE;
				//init._name;
				init._value = data;
				init.document = self;
				init.index = 0;
				auto out = std::make_shared<XmlText>(init);
				((XmlNodeImplInit*)out->internalData())->self = out;
				return out;
			}

			dom::XmlAttributePtr createAttribute(const std::string& name, const std::string& value)
			{
				XmlNodeImplInit init;
				init.type = ATTRIBUTE_NODE;
				init._name = name;
				init._value = value;
				init.document = self;
				init.index = 0;
				auto out = std::make_shared<XmlAttribute>(init);
				((XmlNodeImplInit*)out->internalData())->self = out;
				return out;
			}

			dom::XmlNodeListPtr getElementsByTagName(const std::string& tagName)
			{
				if (!root) return dom::XmlNodeListPtr();
				return root->getElementsByTagName(tagName);
			}

			dom::XmlElementPtr getElementById(const std::string& elementId)
			{
				return nullptr;
			}

			XmlNodePtr find(const std::string& path, const Namespaces& ns)
			{
				return xpath::XPath(path, ns).find(self.lock());
			}
			XmlNodeListPtr findall(const std::string& path, const Namespaces& ns)
			{
				return xpath::XPath(path, ns).findall(self.lock());
			}
		};
	}

	XmlDocumentPtr XmlDocument::create()
	{
		auto out = std::make_shared<impl::XmlDocument>();
		out->self = out;
		return out;
	}

	class DOMParser: public xml::ExpatBase<DOMParser>
	{
		dom::XmlElementPtr elem;
		std::string text;

		void addText()
		{
			if (text.empty()) return;
			if (elem)
				elem->appendChild(doc->createTextNode(text));
			text.clear();
		}
	public:

		dom::XmlDocumentPtr doc;

		bool create(const char* cp)
		{
			doc = dom::XmlDocument::create();
			if (!doc) return false;
			return xml::ExpatBase<DOMParser>::create(cp);
		}

		void onStartElement(const XML_Char *name, const XML_Char **attrs)
		{
			addText();
			auto current = doc->createElement(name);
			if (!current) return;
			for (; *attrs; attrs += 2)
			{
				auto attr = doc->createAttribute(attrs[0], attrs[1]);
				if (!attr) continue;
				current->setAttribute(attr);
			}
			if (elem)
				elem->appendChild(current);
			else
				doc->setDocumentElement(current);
			elem = current;
		}

		void onEndElement(const XML_Char *name)
		{
			addText();
			if (!elem) return;
			dom::XmlNodePtr node = elem->parentNode();
			elem = std::static_pointer_cast<dom::XmlElement>(node);
		}

		void onCharacterData(const XML_Char *pszData, int nLength)
		{
			text += std::string(pszData, nLength);
		}
	};

	XmlDocumentPtr XmlDocument::fromFile(const char* path)
	{
		DOMParser parser;
		if (!parser.create(nullptr)) return nullptr;
		parser.enableElementHandler();
		parser.enableCharacterDataHandler();

		FILE* f = fopen(path, "rb");
		if (!f)
			return nullptr;

		char buffer[8192];
		size_t read;
		while ((read = fread(buffer, 1, sizeof(buffer), f)) > 0)
		{
			if (!parser.parse(buffer, read, false))
			{
				fclose(f);
				return nullptr;
			}
		}
		fclose(f);

		if (!parser.parse(buffer, 0))
			return nullptr;

		return parser.doc;
	}

	void Print(std::ostream& out, dom::XmlNodeListPtr subs, bool ignorews, size_t depth)
	{
		if (subs)
		{
			size_t count = subs->length();
			for(size_t i = 0; i < count; ++i)
				Print(out, subs->item(i), ignorews, depth);
		}
	}

	template <typename T>
	inline std::string qName(std::shared_ptr<T> ptr)
	{
		QName qname = ptr->nodeQName();
		if (qname.nsName.empty()) return qname.localName;
		return "{" + qname.nsName + "}" + qname.localName;
	}

	void Print(std::ostream& out, dom::XmlNodePtr node, bool ignorews, size_t depth)
	{
		dom::XmlNodeListPtr subs = node->childNodes();

		NODE_TYPE type = node->nodeType();
		for (size_t i = 0; i < depth; ++i) out << "    ";

		if (type == TEXT_NODE)
		{
			std::string val = node->nodeValue();
			if (ignorews)
			{
				size_t lo = 0, hi = val.length();
				while (lo < hi && val[lo] && isspace((unsigned char)val[lo])) lo++;
				while (lo < hi && isspace((unsigned char)val[hi-1])) hi--;
				val = val.substr(lo, hi - lo);
				if (val.empty()) return;
			}
			if (val.length() > 80)
				val = val.substr(0, 77) + "[...]";
			out << "# " << val;
		}
		else if (type == ELEMENT_NODE)
		{
			std::string sattrs;
			auto e = std::static_pointer_cast<dom::XmlElement>(node);
			dom::XmlNodeListPtr attrs;
			if (e) attrs = e->getAttributes();
			if (attrs)
			{
				size_t count = attrs->length();
				for(size_t i = 0; i < count; ++i)
				{
					dom::XmlNodePtr node = attrs->item(i);
					sattrs += " " + qName(node) + "='" + node->nodeValue() + "'";
				}
			}

			if (subs && subs->length() == 1)
			{
				dom::XmlNodePtr sub = subs->item(0);
				if (sub && sub->nodeType() == TEXT_NODE)
				{
					out << qName(node);
					if (!sattrs.empty())
						out << "[" + sattrs + " ]";

					std::string val = sub->nodeValue();
					if (ignorews)
					{
						size_t lo = 0, hi = val.length();
						while (val[lo] && isspace((unsigned char)val[lo])) lo++;
						while (lo < hi && isspace((unsigned char)val[hi-1])) hi--;
						val = val.substr(lo, hi - lo);
						if (val.empty()) return;
					}

					if (val.length() > 80)
						val = val.substr(0, 77) + "[...]";

					out << ": " + val + "\n";
					return;
				}
			}
			if (!subs || subs->length() == 0)
			{
					out << qName(node);
					if (!sattrs.empty())
						out << "[" + sattrs + " ]";
					out << "\n";
					return;
			}
			out << "<" + qName(node) + sattrs + ">";
		}
		out << "\n";

		Print(out, subs, ignorews, depth+1);
	}

	XmlNodeListPtr createList(const std::vector<XmlNodePtr>& list)
	{
		try {
		return std::make_shared<impl::XmlNodeList>(list);
		} catch(std::bad_alloc) { return nullptr; }
	}
}
