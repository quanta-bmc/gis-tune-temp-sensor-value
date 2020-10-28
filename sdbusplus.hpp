#include <iostream>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
#define DBUS_PROPERTY_IFACE "org.freedesktop.DBus.Properties"
// Object Mapper related
static constexpr const char* objMapperService =
    "xyz.openbmc_project.ObjectMapper";
static constexpr const char* objMapperPath =
    "/xyz/openbmc_project/object_mapper";
static constexpr const char* objMapperInterface =
    "xyz.openbmc_project.ObjectMapper";
static constexpr const char* getObjectMethod = "GetObject";

namespace util
{

class SDBusPlus
{
  public:
    template <typename T>
    static auto
        setProperty(sdbusplus::bus::bus& bus, const std::string& busName,
                    const std::string& objPath, const std::string& interface,
                    const std::string& property, const T& value)
    {
        std::variant<T> data = value;

        try
        {
            auto methodCall = bus.new_method_call(
                busName.c_str(), objPath.c_str(), DBUS_PROPERTY_IFACE, "Set");

            methodCall.append(interface.c_str());
            methodCall.append(property);
            methodCall.append(data);

            auto reply = bus.call(methodCall);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Set properties fail. ERROR = " << e.what()
                      << std::endl;
            std::cerr << "Object path = " << objPath << std::endl;
            return;
        }
    }

    template <typename Property>
    static auto
        getProperty(sdbusplus::bus::bus& bus, const std::string& busName,
                    const std::string& objPath, const std::string& interface,
                    const std::string& property)
    {
        auto methodCall = bus.new_method_call(busName.c_str(), objPath.c_str(),
                                              DBUS_PROPERTY_IFACE, "Get");

        methodCall.append(interface.c_str());
        methodCall.append(property);

        std::variant<Property> value;

        try
        {
            auto reply = bus.call(methodCall);
            reply.read(value);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Get properties fail.. ERROR = " << e.what()
                      << std::endl;
            std::cerr << "Object path = " << objPath << std::endl;
        }

        return std::get<Property>(value);
    }

    template <typename... Args>
    static auto CallMethod(sdbusplus::bus::bus& bus, const std::string& busName,
                           const std::string& objPath,
                           const std::string& interface,
                           const std::string& method, Args&&... args)
    {
        auto reqMsg = bus.new_method_call(busName.c_str(), objPath.c_str(),
                                          interface.c_str(), method.c_str());
        reqMsg.append(std::forward<Args>(args)...);
        try
        {
            auto respMsg = bus.call(reqMsg);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Call method fail. ERROR = " << e.what() << std::endl;
            std::cerr << "Object path = " << objPath << std::endl;
            return;
        }
    }

    static auto getService(sdbusplus::bus::bus& bus, const std::string& intf,
                           const std::string& path)
    {
        auto mapperCall = bus.new_method_call(objMapperService, objMapperPath,
                                            objMapperInterface, getObjectMethod);

        mapperCall.append(path);
        mapperCall.append(std::vector<std::string>({intf}));

        auto mapperResponseMsg = bus.call(mapperCall);

        std::map<std::string, std::vector<std::string>> mapperResponse;
        mapperResponseMsg.read(mapperResponse);

        if (mapperResponse.begin() == mapperResponse.end())
        {
            throw sdbusplus::exception::SdBusError(
                -EIO, "ERROR in reading the mapper response");
        }

        return mapperResponse.begin()->first;
    }
};

} // namespace util
