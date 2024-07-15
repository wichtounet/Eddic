//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef TYPE_H
#define TYPE_H

#include <string>
#include <memory>
#include <vector>

#include <boost/optional.hpp>

#include "BaseType.hpp"
#include "Platform.hpp"
#include "cpp_utils/assert.hpp"

namespace eddic {

struct GlobalContext;

/*!
 * \class Type
 * \brief A type descriptor.
 * Can describe any type in an EDDI source file.
 */
class Type : public std::enable_shared_from_this<Type> {
    public:
        /*!
         * Deleted copy constructor
         */
        Type(const Type& rhs) = delete;

        /*!
         * Deleted copy assignment operator.
         */
        Type& operator=(const Type& rhs) = delete;

        Type(Type&& rhs) noexcept = default;
        Type& operator=(Type&& rhs) noexcept = default;

        /*!
         * Return the number of elements of the array type.
         * \return The number of elements.
         */
        virtual unsigned int elements() const;

        /*!
         * Return the name of the struct type.
         * \return the name of the struct;
         */
        virtual std::string type() const;

        /*!
         * Return the data type. In the case of an array it is the type of the elements and
         * in the case of a pointer, it is the type of the pointed element.
         * \return the data type
         */
        virtual std::shared_ptr<const Type> data_type() const;

        /*
         * Return the template types of a template type.
         * \return A vector containing all the template types of the current type.
         */
        virtual std::vector<std::shared_ptr<const Type>> template_types() const;

        /*!
         * Indicates if it is an array type
         * \return true if it's an array type, false otherwise.
         */
        virtual bool is_array() const;

        bool is_dynamic_array() const;

        /*!
         * Indicates if it is a custom type
         * \return true if it's a custom type, false otherwise.
         */
        virtual bool is_custom_type() const;

        /*!
         * Indicates if it is a structure type. It can be a custom type or a template type.
         * \return true if it's a structure type, false otherwise.
         */
        bool is_structure() const;

        /*!
         * Indicates if it is a standard type
         * \return true if it's a standard type, false otherwise.
         */
        virtual bool is_standard_type() const;

        /*!
         * Indicates if it is a pointer type
         * \return true if it's a pointer type, false otherwise.
         */
        virtual bool is_pointer() const;

        virtual bool is_incomplete() const;

        virtual bool has_elements() const;

        /*!
         * Indicates if the type is const
         * \return true if the type is const, false otherwise.
         */
        virtual bool is_const() const;

        /*!
         * Indicates if the type is a template type.
         * \return true if the type is a template type, false otherwise.
         */
        virtual bool is_template_type() const;

        /*!
         * Return the size of the type in memory in octets.
         * \return the size of the type, in octets.
         */
        unsigned int size() const {
            if (!size_) {
            std::cout << "INvalid size for " << mangle() << std::endl;
            }
            cpp_assert(size_ > 0, "Invalid size");
            return size_;
        }

        /*!
         * Return the mangled name of the type.
         * \return The mangled name of the type.
         */
        std::string mangle() const;

        friend bool operator==(std::shared_ptr<const Type> lhs, std::shared_ptr<const Type> rhs);
        friend bool operator!=(std::shared_ptr<const Type> lhs, std::shared_ptr<const Type> rhs);

    protected:
        /*!
         * Construct a new Type.
         */
        explicit Type(unsigned int size) : size_(size) {}

        /*!
         * Return the base type of a standard type
         * \return the base type.
         */
        virtual BaseType base() const;

        unsigned size_ = 0;
};

/*!
 * \class StandardType
 * \brief A standard type descriptor.
 */
class StandardType : public Type {
    private:
        BaseType base_type;
        bool const_;

        BaseType base() const override;

    public:
        StandardType(Platform platform, BaseType type, bool const_);

        /*!
         * Deleted copy constructor
         */
        StandardType(const StandardType& rhs) = delete;

        /*!
         * Deleted copy assignment operator.
         */
        StandardType& operator=(const StandardType& rhs) = delete;

        bool is_standard_type() const override;
        bool is_const() const override;
};

/*!
 * \class CustomType
 * \brief A custom type descriptor.
 */
class CustomType : public Type {
    private:
        std::string m_type;

    public:
        CustomType(const GlobalContext & context, const std::string& type);

        /*!
         * Deleted copy constructor
         */
        CustomType(const CustomType& rhs) = delete;

        /*!
         * Deleted copy assignment operator.
         */
        CustomType& operator=(const CustomType& rhs) = delete;

        std::string type() const override;

        bool is_custom_type() const override;
};

/*!
 * \struct ArrayType
 * \brief An array type descriptor.
 */
struct ArrayType : public Type {
    private:
        std::shared_ptr<const Type> sub_type;
        boost::optional<unsigned int> m_elements;

    public:
        ArrayType(const std::shared_ptr<const Type> & sub_type);
        ArrayType(const std::shared_ptr<const Type> & sub_type, int size);

        /*!
         * Deleted copy constructor
         */
        ArrayType(const ArrayType& rhs) = delete;

        /*!
         * Deleted copy assignment operator.
         */
        ArrayType& operator=(const ArrayType& rhs) = delete;

        unsigned int elements() const override;
        bool has_elements() const override;

        std::shared_ptr<const Type> data_type() const override;

        bool is_array() const override;
};

/*!
 * \struct PointerType
 * \brief A pointer type descriptor.
 */
struct PointerType : public Type {
    private:
        std::shared_ptr<const Type> sub_type;
        bool incomplete = false;

    public:
        PointerType();
        explicit PointerType(const std::shared_ptr<const Type> & sub_type);

        /*!
         * Deleted copy constructor
         */
        PointerType(const PointerType& rhs) = delete;

        /*!
         * Deleted copy assignment operator.
         */
        PointerType& operator=(const PointerType& rhs) = delete;

        std::shared_ptr<const Type> data_type() const override;

        bool is_pointer() const override;
        bool is_incomplete() const override;
};

/*!
 * \struct TemplateType
 * \brief A template type descriptor.
 */
struct TemplateType : public Type {
    private:
        std::string main_type;
        std::vector<std::shared_ptr<const Type>> sub_types;

    public:
        TemplateType(const GlobalContext & context, const std::string & main_type, const std::vector<std::shared_ptr<const Type>> & sub_types);

        /*!
         * Deleted copy constructor
         */
        TemplateType(const PointerType& rhs) = delete;

        /*!
         * Deleted copy assignment operator.
         */
        TemplateType& operator=(const PointerType& rhs) = delete;

        std::string type() const override;
        std::vector<std::shared_ptr<const Type>> template_types() const override;

        bool is_template_type() const override;
};

/* Relational operators  */

bool operator==(std::shared_ptr<const Type> lhs, std::shared_ptr<const Type> rhs);
bool operator!=(std::shared_ptr<const Type> lhs, std::shared_ptr<const Type> rhs);

extern std::shared_ptr<const Type> BOOL;
extern std::shared_ptr<const Type> INT;
extern std::shared_ptr<const Type> CHAR;
extern std::shared_ptr<const Type> FLOAT;
extern std::shared_ptr<const Type> STRING;
extern std::shared_ptr<const Type> VOID;
extern std::shared_ptr<const Type> POINTER;

void init_global_types(Platform platform);

/*!
 * \brief Parse the given type into an EDDI std::shared_ptr<Type>.
 *
 * \param context The current global context
 * \param type The type to parse.
 */
std::shared_ptr<const Type> new_type(const GlobalContext & context, const std::string& type, bool const_ = false);

/*!
 * Create a new array type of the given type.
 * \param data_type The type of data hold by the array.
 * \return the created type;
 */
std::shared_ptr<const Type> new_array_type(std::shared_ptr<const Type> data_type);

/*!
 * Create a new array type of the given type.
 * \param data_type The type of data hold by the array.
 * \param size The number of elements, if known.
 * \return the created type;
 */
std::shared_ptr<const Type> new_array_type(std::shared_ptr<const Type> data_type, int size);

/*!
 * Create a new pointer type of the given type.
 * \param data_type The type of data pointed.
 * \return the created type;
 */
std::shared_ptr<const Type> new_pointer_type(std::shared_ptr<const Type> data_type);

std::shared_ptr<const Type> new_template_type(const GlobalContext & context, std::string data_type, std::vector<std::shared_ptr<const Type>> template_types);

/*!
 * Indicates if the given type is a standard type or not.
 * \param type The type to test.
 * \return true if the type is standard, false otherwise.
 */
bool is_standard_type(const std::string& type);

} //end of eddic

#endif
