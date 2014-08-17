#ifndef SMC_XML_ATTRIBUTES_HPP
#define SMC_XML_ATTRIBUTES_HPP
#include "global_game.hpp"
#include "errors.hpp"
#include "property_helper.hpp"

namespace SMC {

    class XmlAttributes: public std::map<std::string, std::string> {
    public:
        // If the given key `attribute_name' has the value `filename_old'
        //(either with or without the pixmaps dir), replace it with `filename_new'.
        void relocate_image(const std::string& filename_old, const std::string& filename_new, const std::string& attribute_name = "image");

        // Returns true if the given key exists, false otherwise.
        bool exists(const std::string& key);

        // If the given `key' exists, return its value. Otherwise return `defaultvalue'.
        // For strings, an this template is overriden to do no conversion at all.
        template <typename T>
        T fetch(const std::string& key, T defaultvalue)
        {
            if (exists(key))
                return string_to_type<T>((*this)[key]);
            else
                return defaultvalue;
        }

        // If the given `key' exists, returns its value as the
        // type indicated by the template. If it doesn’t exist,
        // throw an instance of
        template <typename T>
        T retrieve(const std::string& key)
        {
            if (exists(key))
                return string_to_type<T>((*this)[key]);
            else
                throw (XmlKeyDoesNotExist(key));
        }
    };

    template<>
    inline std::string XmlAttributes::fetch(const std::string& key, std::string defaultvalue)
    {
        if (exists(key))
            return (*this)[key];
        else
            return defaultvalue;
    }

    template<>
    inline const char* XmlAttributes::fetch(const std::string& key, const char* defaultvalue)
    {
        if (exists(key))
            return (*this)[key].c_str();
        else
            return defaultvalue;
    }
}

#endif
