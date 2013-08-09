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

#ifndef __DOM_HPP__
#define __DOM_HPP__

#include <memory>
#include <string>

namespace dom
{
	struct XmlDocument;
	struct XmlNode;
	struct XmlNodeList;
	struct XmlElement;
	struct XmlAttribute;
	struct XmlText;

	typedef std::shared_ptr<XmlDocument> XmlDocumentPtr;
	typedef std::shared_ptr<XmlNode> XmlNodePtr;
	typedef std::shared_ptr<XmlNodeList> XmlNodeListPtr;
	typedef std::shared_ptr<XmlElement> XmlElementPtr;
	typedef std::shared_ptr<XmlAttribute> XmlAttributePtr;
	typedef std::shared_ptr<XmlText> XmlTextPtr;

	struct NSData
	{
		const char* key;
		const char* ns;
	};

	typedef NSData* Namespaces;

	struct QName
	{
		std::string nsName;
		std::string localName;

		bool operator == (const QName& right) const
		{
			return nsName == right.nsName && localName == right.localName;
		}
		bool operator != (const QName& right) const
		{
			return !(*this == right);
		}
	};

	inline std::ostream& operator << (std::ostream& o, const QName& qname)
	{
		if (!qname.nsName.empty())
			o << "{" << qname.nsName << "}";
		return o << qname.localName;
	}

	enum NODE_TYPE
	{
		DOCUMENT_NODE  = 0,
		ELEMENT_NODE   = 1,
		ATTRIBUTE_NODE = 2,
		TEXT_NODE      = 3
	};

	struct XmlNode
	{
		virtual ~XmlNode() {}
		virtual std::string nodeName() const = 0;
		virtual const QName& nodeQName() const = 0;
		virtual std::string nodeValue() const = 0;
		virtual std::string stringValue() { return nodeValue(); } // nodeValue for TEXT, ATTRIBUTE, and - coincidently - DOCUMENT; innerText for ELEMENT; used in xpath
		virtual void nodeValue(const std::string& val) = 0;
		virtual NODE_TYPE nodeType() const = 0;

		virtual XmlNodePtr parentNode() = 0;
		virtual XmlNodeListPtr childNodes() = 0;
		virtual XmlNodePtr firstChild() = 0;
		virtual XmlNodePtr lastChild() = 0;
		virtual XmlNodePtr previousSibling() = 0;
		virtual XmlNodePtr nextSibling() = 0;

		virtual XmlDocumentPtr ownerDocument() = 0;
		virtual bool appendChild(XmlNodePtr newChild) = 0;
		virtual void* internalData() = 0;

		virtual XmlNodePtr find(const std::string& path, const Namespaces& ns) = 0;
		virtual XmlNodePtr find(const std::string& path) { return find(path, nullptr); }
		virtual XmlNodeListPtr findall(const std::string& path, const Namespaces& ns) = 0;
	};

	struct XmlDocument: XmlNode
	{
		static XmlDocumentPtr create();
		static XmlDocumentPtr fromFile(const char* path);
		virtual ~XmlDocument() {}

		virtual XmlElementPtr documentElement() = 0;
		virtual void setDocumentElement(XmlElementPtr elem) = 0;

		virtual XmlElementPtr createElement(const std::string& tagName) = 0;
		virtual XmlTextPtr createTextNode(const std::string& data) = 0;
		virtual XmlAttributePtr createAttribute(const std::string& name, const std::string& value) = 0;

		virtual XmlNodeListPtr getElementsByTagName(const std::string& tagName) = 0;
		virtual XmlElementPtr getElementById(const std::string& elementId) = 0;
	};

	struct XmlNodeList
	{
		virtual ~XmlNodeList() {}
		virtual XmlNodePtr item(size_t index) = 0;
		XmlElementPtr element(size_t index)
		{
			XmlNodePtr i = item(index);
			if (i.get() && i->nodeType() == ELEMENT_NODE)
				return std::static_pointer_cast<XmlElement>(i);
			return XmlElementPtr();
		}
		XmlTextPtr text(size_t index)
		{
			XmlNodePtr i = item(index);
			if (i.get() && i->nodeType() == TEXT_NODE)
				return std::static_pointer_cast<XmlText>(i);
			return XmlTextPtr();
		}
		virtual size_t length() const = 0;
	};

	struct XmlElement: XmlNode
	{
		virtual std::string tagName() const { return nodeName(); }
		virtual std::string stringValue() { return innerText(); }
		virtual std::string getAttribute(const std::string& name) = 0;
		virtual XmlAttributePtr getAttributeNode(const std::string& name) = 0;
		virtual bool setAttribute(XmlAttributePtr attr) = 0;
		virtual XmlNodeListPtr getAttributes() = 0;
		virtual bool hasAttribute(const std::string& name) = 0;
		virtual XmlNodeListPtr getElementsByTagName(const std::string& tagName) = 0;
		virtual std::string innerText() = 0;
	};

	struct XmlAttribute: XmlNode
	{
		virtual std::string name() const { return nodeName(); }
		virtual std::string value() const { return nodeValue(); }
		virtual void value(const std::string& val) { nodeValue(val); }
		virtual XmlElementPtr ownerElement()
		{
			return std::static_pointer_cast<XmlElement>(parentNode());
		}
	};

	struct XmlText: XmlNode
	{
		virtual std::string data() const { return nodeValue(); }
	};

	void Print(std::ostream& out, XmlNodePtr node, bool ignorews = false, size_t depth = 0);
	void Print(std::ostream& out, XmlNodeListPtr subs, bool ignorews = false, size_t depth = 0);
}

#endif // __DOM_HPP__