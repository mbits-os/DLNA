#ifndef __EXPAT_HPP__
#define __EXPAT_HPP__

#include <expat.h>

#define USER_DATA static_cast<Final*>(userData)

namespace xml
{
	template <class Final>
	class ExpatBase
	{
	public:
		ExpatBase()
			: m_parser(nullptr)
		{
		}
		~ExpatBase()
		{
			destroy();
		}

		bool create(const XML_Char* encoding = nullptr, const XML_Char* sep = nullptr)
		{
			destroy();
			if (encoding != nullptr && encoding [0] == 0)
				encoding = nullptr;

			if (sep != nullptr && sep [0] == 0)
				sep = nullptr;

			m_parser = XML_ParserCreate_MM(encoding, nullptr, sep);
			if (m_parser == nullptr)
				return false;

			Final* pThis = static_cast<Final*>(this);
			pThis->onPostCreate();

			XML_SetUserData(m_parser, (void*) pThis);
			return true;
		}

		bool isCreated() const { return m_parser != nullptr; }

		void destroy()
		{
			if (m_parser)
				XML_ParserFree(m_parser);
			m_parser = nullptr;
		}

		bool parse(const char* buffer, int length = -1, bool isFinal = true)
		{
			if (length < 0)
				length = strlen(buffer);

			return XML_Parse(m_parser, buffer, length, isFinal) != 0;
		}
		bool parseBuffer(int length, bool isFinal = true)
		{
			return XML_ParseBuffer(m_parser, length, isFinal) != 0;
		}
		void* getBuffer(int length)
		{
			return XML_GetBuffer(m_parser, length);
		}
		void enableStartElementHandler(bool enable = true)
		{
			XML_SetStartElementHandler(m_parser, enable ? startElementHandler : nullptr);
		}
		void enableEndElementHandler(bool enable = true)
		{
			XML_SetEndElementHandler(m_parser, enable ? endElementHandler : nullptr);
		}
		void enableElementHandler(bool enable = true)
		{
			enableStartElementHandler(enable);
			enableEndElementHandler(enable);
		}
		void enableCharacterDataHandler(bool enable = true)
		{
			XML_SetCharacterDataHandler(m_parser, enable ? characterDataHandler : nullptr);
		}
		void enableProcessingInstructionHandler(bool enable = true)
		{
			XML_SetProcessingInstructionHandler(m_parser, enable ? ProcessingInstructionHandler : nullptr);
		}
		void enableCommentHandler(bool enable = true)
		{
			XML_SetCommentHandler(m_parser, enable ? commentHandler : nullptr);
		}
		void enableStartCdataSectionHandler(bool enable = true)
		{
			XML_SetStartCdataSectionHandler(m_parser,
				enable ? startCdataSectionHandler : nullptr);
		}
		void enableEndCdataSectionHandler(bool enable = true)
		{
			XML_SetEndCdataSectionHandler(m_parser,
				enable ? endCdataSectionHandler : nullptr);
		}
		void enableCdataSectionHandler(bool enable = true)
		{
			enableStartCdataSectionHandler(enable);
			enableEndCdataSectionHandler(enable);
		}
		void enableDefaultHandler(bool enable = true, bool expand = true)
		{
			if (expand)
			{
				XML_SetDefaultHandlerExpand(m_parser,
					enable ? defaultHandler : nullptr);
			}
			else
				XML_SetDefaultHandler(m_parser, enable ? defaultHandler : nullptr);
		}
		void enableExternalEntityRefHandler(bool enable = true)
		{
			XML_SetExternalEntityRefHandler(m_parser,
				enable ? externalEntityRefHandler : nullptr);
		}
		void enableUnknownEncodingHandler(bool enable = true)
		{
			Final* pThis = static_cast<Final*>(this);
			XML_SetUnknownEncodingHandler(m_parser,
				enable ? unknownEncodingHandler : nullptr,
				enable ? (void*) pThis : nullptr);
		}
		void enableStartNamespaceDeclHandler(bool enable = true)
		{
			XML_SetStartNamespaceDeclHandler(m_parser,
				enable ? startNamespaceDeclHandler : nullptr);
		}
		void enableEndNamespaceDeclHandler(bool enable = true)
		{
			XML_SetEndNamespaceDeclHandler(m_parser,
				enable ? endNamespaceDeclHandler : nullptr);
		}
		void enableNamespaceDeclHandler(bool enable = true)
		{
			enableStartNamespaceDeclHandler(enable);
			enableEndNamespaceDeclHandler(enable);
		}
		void enableXmlDeclHandler(bool enable = true)
		{
			XML_SetXmlDeclHandler(m_parser, enable ? xmlDeclHandler : nullptr);
		}
		void enableStartDoctypeDeclHandler(bool enable = true)
		{
			XML_SetStartDoctypeDeclHandler(m_parser,
				enable ? startDoctypeDeclHandler : nullptr);
		}
		void enableEndDoctypeDeclHandler(bool enable = true)
		{
			XML_SetEndDoctypeDeclHandler(m_parser,
				enable ? endDoctypeDeclHandler : nullptr);
		}
		void enableDoctypeDeclHandler(bool enable = true)
		{
			enableStartDoctypeDeclHandler(enable);
			enableEndDoctypeDeclHandler(enable);
		}

		enum XML_Error getErrorCode()
		{
			return XML_GetErrorCode(m_parser);
		}
		long getCurrentByteIndex()
		{
			return XML_GetCurrentByteIndex(m_parser);
		}
		int getCurrentLineNumber()
		{
			return XML_GetCurrentLineNumber(m_parser);
		}
		int getCurrentColumnNumber()
		{
			return XML_GetCurrentColumnNumber(m_parser);
		}
		int getCurrentByteCount()
		{
			return XML_GetCurrentByteCount(m_parser);
		}
		const char* getInputContext(int* offset, int* size)
		{
			return XML_GetInputContext(m_parser, offset, size);
		}
		const XML_LChar* getErrorString()
		{
			return XML_ErrorString(getErrorCode());
		}

		static const XML_LChar* getExpatVersion() { return XML_ExpatVersion(); }
		static XML_Expat_Version getExpatVersionInfo() { return XML_ExpatVersionInfo(); }
		static const XML_LChar* getErrorString(enum XML_Error error) { return XML_ErrorString(error); }

		void onStartElement(const XML_Char* name, const XML_Char** attrs) {}
		void onEndElement(const XML_Char* name) {}
		void onCharacterData(const XML_Char* data, int length) {}
		void onProcessingInstruction(const XML_Char* target, const XML_Char* data) {}
		void onComment(const XML_Char* data) {}
		void onStartCdataSection() {}
		void onEndCdataSection() {}
		void onDefault(const XML_Char* data, int length) {}
		bool onExternalEntityRef(const XML_Char* context, const XML_Char* base, const XML_Char* systemID, const XML_Char* publicID)
		{
			return false;
		}
		bool onUnknownEncoding(const XML_Char* name, XML_Encoding* info)
		{
			return false;
		}
		void onStartNamespaceDecl(const XML_Char* prefix, const XML_Char* uri) {}
		void onEndNamespaceDecl(const XML_Char* prefix) {}
		void onXmlDecl(const XML_Char* version, const XML_Char* encoding, bool standalone) {}
		void onStartDoctypeDecl(const XML_Char* doctypeName, const XML_Char* systemId, const XML_Char* publicID, bool hasInternalSubset) {}
		void onEndDoctypeDecl() {}

	protected:
		void onPostCreate() {}

		static void startElementHandler(void* userData, const XML_Char* name, const XML_Char** attrs)
		{
			USER_DATA->onStartElement(name, attrs);
		}
		static void endElementHandler(void* userData, const XML_Char* name)
		{
			USER_DATA->onEndElement(name);
		}
		static void characterDataHandler(void* userData, const XML_Char* data, int length)
		{
			USER_DATA->onCharacterData(data, length);
		}
		static void ProcessingInstructionHandler(void* userData, const XML_Char* target, const XML_Char* data)
		{
			USER_DATA->onProcessingInstruction(target, data);
		}
		static void commentHandler(void* userData, const XML_Char* data)
		{
			USER_DATA->onComment(data);
		}
		static void startCdataSectionHandler(void* userData)
		{
			USER_DATA->onStartCdataSection();
		}
		static void endCdataSectionHandler(void* userData)
		{
			USER_DATA->onEndCdataSection();
		}
		static void defaultHandler(void* userData, const XML_Char* data, int length)
		{
			USER_DATA->onDefault(data, length);
		}
		static int externalEntityRefHandler(void* userData, const XML_Char* context, const XML_Char* base, const XML_Char* systemID, const XML_Char* publicID)
		{
			return USER_DATA->onExternalEntityRef(context, base, systemID, publicID) ? 1 : 0;
		}
		static int unknownEncodingHandler(void* userData, const XML_Char* name, XML_Encoding* info)
		{
			return USER_DATA->onUnknownEncoding(name, info) ? XML_STATUS_OK : XML_STATUS_ERROR;
		}
		static void startNamespaceDeclHandler(void* userData, const XML_Char* prefix, const XML_Char* uri)
		{
			USER_DATA->onStartNamespaceDecl(prefix, uri);
		}
		static void endNamespaceDeclHandler(void* userData, const XML_Char* prefix)
		{
			USER_DATA->onEndNamespaceDecl(prefix);
		}
		static void xmlDeclHandler(void* userData, const XML_Char* version, const XML_Char* encoding, int standalone)
		{
			USER_DATA->onXmlDecl(version, encoding, standalone != 0);
		}
		static void startDoctypeDeclHandler(void* userData, const XML_Char* doctypeName, const XML_Char* systemId, const XML_Char* publicID, int hasInternalSubset)
		{
			USER_DATA->onStartDoctypeDecl(doctypeName, systemId, publicID, hasInternalSubset != 0);
		}
		static void endDoctypeDeclHandler(void* userData)
		{
			USER_DATA->onEndDoctypeDecl();
		}

		XML_Parser		m_parser;
	};
}
#undef USER_DATA

#endif //__EXPAT_HPP__
