// This file is autogenerated.
// Do not edit.

#ifndef __SSDP_SERVICE_IMPL_HPP__
#define __SSDP_SERVICE_IMPL_HPP__

#include <string>
#include <http/response.hpp>
#include <ssdp/device.hpp>
#include <dom.hpp>

namespace net { namespace ssdp { namespace import {

	template <typename T>
	struct type_info;

	using error::error_code;
	typedef int i4;
	typedef unsigned int ui4;

	struct uri : std::string
	{
		uri() : std::string() {}
		explicit uri(std::string && v) : std::string(std::move(v)) {}
		explicit uri(const std::string & v) : std::string(v) {}
		uri& operator=(std::string && v)
		{
			std::string::operator=(std::move(v));
			return *this;
		}
	};

	template <>
	struct type_info<uri> {
		static uri unknown_value() { return uri(); }

		static std::string to_string(const uri& rhs)
		{
			return rhs;
		};

		static uri from_string(const std::string& rhs)
		{
			return uri(rhs);
		};
		static void get_config(std::ostream& o) { o << "			<dataType>uri</dataType>\n"; }
	};

	template <>
	struct type_info<std::string> {
		static std::string unknown_value() { return std::string(); }

		static std::string to_string(const std::string& rhs)
		{
			return rhs;
		};

		static std::string from_string(const std::string& rhs)
		{
			return std::string(rhs);
		};
		static void get_config(std::ostream& o) { o << "			<dataType>string</dataType>\n"; }
	};

	template <typename T>
	struct type_info_int {
		static T unknown_value() { return 0; }

		static std::string to_string(T rhs)
		{
			return std::to_string(rhs);
		};

		static T from_string(const std::string& rhs)
		{
			std::istringstream i(rhs);
			T val;
			i >> val;
			return val;
		};
	};

	template <>
	struct type_info<i4>: type_info_int<i4>
	{
		static void get_config(std::ostream& o) { o << "			<dataType>i4</dataType>\n"; }
	};

	template <>
	struct type_info<ui4>: type_info_int<ui4>
	{
		static void get_config(std::ostream& o) { o << "			<dataType>ui4</dataType>\n"; }
	};

	struct SOAP
	{
		static const char* BODY_START()
		{
			return R"(<?xml version="1.0" encoding="utf-8"?>)" "\n"
				R"(<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">)" "\n"
				R"(<s:Body>)" "\n";
		}

		static char* BODY_STOP()
		{
			return "\n</s:Body>\n</s:Envelope>\n";
		}

		static void soap_answer(const char* method, const char* service_urn, http::response& response, const std::string& body)
		{
			auto & header = response.header();
			header.clear({"-", 0, 0});
			header.append("content-type", "text/xml; charset=\"utf-8\"");
			std::ostringstream o;
			o
				<< BODY_START()
				<< "<u:" << method << "Response xmlns:u=\"" << service_urn << "\">\n"
				<< body
				<< "\n</u:" << method << "Response>"
				<< BODY_STOP();

			response.content(http::content::from_string(o.str()));
		}

		static void quick404(http::response& response)
		{
			auto & header = response.header();
			header.m_status = 404;
		}

		static void quick500(http::response& response)
		{
			auto & header = response.header();
			header.m_status = 500;
		}

		static void upnp_error(http::response& response, const ssdp::service_error& e)
		{
			auto & header = response.header();
			header.m_status = 500;
			header.append("content-type", "text/xml; charset=\"utf-8\"");
			std::ostringstream o;
			o
				<< BODY_START()
				<< "  <s:Fault>\n"
				"    <faultcode>s:Client</faultcode>\n"
				"    <faultstring>UPnPError</faultstring>\n"
				"    <detail>\n"
				"      <UPnPError xmlns=\"urn:schemas-upnp-org:control-1-0\">\n"
				"        <errorCode>" << e.code() << "</errorCode>\n"
				"        <errorDescription>" << e.message() << "</errorDescription>\n"
				"      </UPnPError>\n"
				"    </detail>\n"
				"  </s:Fault>"
				<< BODY_STOP();

			response.content(http::content::from_string(o.str()));
		}
	};

	struct variable_base
	{
		virtual ~variable_base() {}
		virtual void get_config(std::ostream& o) = 0;
	};
	typedef std::shared_ptr<variable_base> variable_ptr;

	template <typename T>
	struct variable : variable_base
	{
		std::string m_name;
		bool m_signalable;
		variable(const std::string& name, bool signalable) : m_name(name), m_signalable(signalable) {}

		void get_config(std::ostream& o) override
		{
			o <<
				"		<stateVariable sendEvents=\"" << (m_signalable ? "yes" : "no") << "\">\n"
				"			<name>" << m_name << "</name>\n";
			type_info<T>::get_config(o);
			o <<
				"		</stateVariable>\n";
		}
	};

	template <typename Proxy>
	struct method_base
	{
		typedef Proxy proxy_t;
		virtual ~method_base() {}

		virtual const std::string& name() const = 0;
		virtual bool call(proxy_t* self, const http::http_request& req, const dom::XmlDocumentPtr& doc, http::response& response) = 0;
		virtual void get_config(std::ostream& o) = 0;
	};

	template <typename Proxy, typename Request, typename Response>
	struct method_info: method_base<Proxy>
	{
		typedef Proxy proxy_t;
		typedef Response response_t;
		typedef Request request_t;
		typedef error_code(proxy_t::* method_t)(const http::http_request& req, const request_t&, response_t&);

		struct accessor_base
		{
			virtual ~accessor_base() {};

			virtual void load(const dom::XmlNodePtr& src, request_t& dst) = 0;
			virtual void clean(response_t& dst) = 0;
			virtual void store(const response_t& src, std::ostream& out) = 0;
			virtual void get_config(std::ostream& o) = 0;

			void get_config(std::ostream& o, const char* name, const char* dir, const char* ref)
			{
				o <<
					"				<argument>\n"
					"					<name>" << name << "</name>\n"
					"					<direction>" << dir << "</direction>\n"
					"					<relatedStateVariable>" << ref << "</relatedStateVariable>\n"
					"				</argument>\n";
			}
		};

		template <typename T>
		struct accessor_load: accessor_base
		{
			typedef T field_t;
			typedef field_t request_t::* ptr_t;
			typedef variable<field_t> variable_t;

			std::string m_name;
			ptr_t       m_field;
			variable_t& m_ref;
			accessor_load(const std::string& name, variable_t& ref, ptr_t field) : m_name(name), m_ref(ref), m_field(field) {}

			void clean(response_t& dst) override {}
			void store(const response_t& src, std::ostream& out) override {}
			void load(const dom::XmlNodePtr& src, request_t& dst) override
			{
				auto node = src->find(m_name);
				if (!node)
					dst.*m_field = type_info<field_t>::unknown_value();
				else
					dst.*m_field = type_info<field_t>::from_string(node->stringValue());
			}
			void get_config(std::ostream& o) override
			{
				accessor_base::get_config(o, m_name.c_str(), "in", m_ref.m_name.c_str());
			}
		};

		template <typename T>
		struct accessor_store : accessor_base
		{
			typedef T field_t;
			typedef field_t response_t::* ptr_t;
			typedef variable<field_t> variable_t;

			std::string m_name;
			ptr_t       m_field;
			variable_t& m_ref;
			accessor_store(const std::string& name, variable_t& ref, ptr_t field) : m_name(name), m_ref(ref), m_field(field) {}

			void load(const dom::XmlNodePtr& src, request_t& dst) override {}
			void clean(response_t& dst) override
			{
				dst.*m_field = type_info<field_t>::unknown_value();
			}
			void store(const response_t& src, std::ostream& out) override
			{
				out << "<" << m_name << ">" << type_info<field_t>::to_string(src.*m_field) << "</" << m_name << ">";
			}
			void get_config(std::ostream& o) override
			{
				accessor_base::get_config(o, m_name.c_str(), "out", m_ref.m_name.c_str());
			}
		};

		typedef std::shared_ptr<accessor_base> accessor_ptr;

		std::string m_name;
		method_t m_method;
		std::vector<accessor_ptr> m_accessors;

		method_info(const std::string& name, method_t method) : m_name(name), m_method(method) {}

		template <typename Field>
		method_info& input(const std::string& name, variable<Field>& ref, Field request_t::* field)
		{
			m_accessors.push_back(std::make_shared<accessor_load<Field>>(name, ref, field));
			return *this;
		}

		template <typename Field>
		method_info& output(const std::string& name, variable<Field>& ref, Field response_t::* field)
		{
			m_accessors.push_back(std::make_shared<accessor_store<Field>>(name, ref, field));
			return *this;
		}

		void load(const dom::XmlNodePtr& src, request_t& dst)
		{
			for (auto&& accessor : m_accessors)
				accessor->load(src, dst);
		}

		void clean(response_t& dst)
		{
			for (auto && accessor : m_accessors)
				accessor->clean(dst);
		}

		void store(response_t& src, std::ostream& out)
		{
			for (auto && accessor : m_accessors)
				accessor->store(src, out);
		}

		const std::string& name() const override { return m_name; }

		bool call(proxy_t* self, const http::http_request& req, const dom::XmlDocumentPtr& doc, http::response& response) override
		{
			request_t call_req;
			response_t call_resp;

			error_code result = error::cannot_process_the_request;

			dom::NSData ns [] = { { "s", "http://schemas.xmlsoap.org/soap/envelope/" }, { "upnp", self->get_type() } };
			auto elem = doc->find("/s:Envelope/s:Body/upnp:" + m_name, ns);
			if (elem)
			{
				clean(call_resp);
				load(elem, call_req);
				result = (self->*m_method)(req, call_req, call_resp);
				if (result == error::no_error)
				{
					std::ostringstream out;
					store(call_resp, out);
					SOAP::soap_answer(m_name.c_str(), self->get_type(), response, out.str());
				}
				else if (result == error::not_implemented)
				{
					return false;
				}
				else if (result == error::internal_error)
				{
					SOAP::quick500(response);
				}
				return true;
			}

			SOAP::upnp_error(response, result);
			return true;
		}

		void get_config(std::ostream& o) override
		{
			o <<
				"		<action>\n"
				"			<name>" << m_name << "</name>\n"
				"			<argumentList>\n";

			for (auto && accessor : m_accessors)
				accessor->get_config(o);

			o <<
				"			</argumentList>\n"
				"		</action>\n";
		}
	};

	template <typename Proxy, typename Interface, size_t major_val, size_t minor_val>
	struct ServerImpl : Interface
	{
		typedef method_base<Proxy> method_t;
		typedef std::shared_ptr<method_t> method_ptr_t;
		typedef std::vector<method_ptr_t> methods_t;
		typedef std::vector<variable_ptr> variables_t;

		methods_t m_methods;
		variables_t m_variables;

		bool answer(const std::string& name, const http::http_request& req, const dom::XmlDocumentPtr& doc, http::response& response) override
		{
			for (auto&& method: m_methods)
			{
				if (method->name() == name)
					return method->call(static_cast<Proxy*>(this), req, doc, response);
			}

			return false;
		}
		template <typename Request, typename Response>
		method_info<Proxy, Request, Response>& add_method(const std::string& name, error_code (Proxy::* method)(const http::http_request&, const Request&, Response&))
		{
			auto m = std::make_shared<method_info<Proxy, Request, Response>>(name, method);
			m_methods.push_back(m);
			return *m.get();
		}

		template <typename T>
		variable<T>& add_type(const std::string& name)
		{
			auto var = std::make_shared<variable<T>>(name, false);
			m_variables.push_back(var);
			return *var.get();
		}

		template <typename T>
		variable<T>& add_event(const std::string& name)
		{
			auto var = std::make_shared<variable<T>>(name, true);
			m_variables.push_back(var);
			return *var.get();
		}

		std::string x();
		std::string get_configuration() const override
		{
			std::ostringstream o;
			o
				<< "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
				"<scpd xmlns=\"urn:schemas-upnp-org:service-1-0\">\n"
				"	<specVersion>\n"
				"		<major>" << major_val << "</major>\n"
				"		<minor>" << minor_val << "</minor>\n"
				"	</specVersion>\n"
				"	<actionList>\n";

			for (auto&& m: m_methods)
				m->get_config(o);

			o <<
				"	</actionList>\n"
				"	<serviceStateTable>\n";

			for (auto && v : m_variables)
				v->get_config(o);

			o <<
				"	</serviceStateTable>\n"
				"</scpd>\n";

			return o.str();
		}
	};
}}}

#endif // __SSDP_SERVICE_IMPL_HPP__
