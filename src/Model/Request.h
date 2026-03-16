#ifndef REQUEST_H
#define REQUEST_H

#include <string>

enum Methods
{
    GET, POST, DELETE, PUT, UNKWOWN
};

class Request
{
public:
    /**
    * Class constructor
    */
    Request():
        method_(UNKWOWN)
        , path_("")
        , version_("")
        , host_("")
        , user_agent_("")
    {}

    /**
    * Class constructor with parameter
    */
    Request(Methods method, std::string path, std::string version, std::string host, std::string user_agent):
        method_(method)
        , path_(path)
        , version_(version)
        , host_(host)
        , user_agent_(user_agent)
    {}

    /**
     * @brief string2Enum convert from string to enum
     * @param meth string about method
     * @return enum relative to request
     */
    Methods string2Enum(std::string& meth)
    {
        if (meth == "GET")
            return GET;

        else if (meth == "POST")
            return POST;

        else if (meth == "DELETE")
            return DELETE;

        else if (meth == "PUT")
            return PUT;

        return UNKWOWN;
    }

    Methods getMethod()
    {
        return method_;
    }

    void setMethod(std::string& meth)
    {
        method_ = string2Enum(meth);
    }


    std::string getPath()
    {
        return path_;
    }

    void setPath(std::string& path)
    {
        path_ = path;
    }


    std::string getVersion()
    {
        return version_;
    }

    void setVersion(std::string& version)
    {
        version_ = version;
    }

    std::string getHost()
    {
        return host_;
    }

    void setHost(std::string& host)
    {
        host_ = host;
    }

    std::string getUserAgent()
    {
        return user_agent_;
    }

    void setUserAgent(std::string& user_agent)
    {
        user_agent_ = user_agent;
    }


private:
    Methods method_;            ///< method about request
    std::string path_;          ///< path about request
    std::string version_;       ///< version about request: HTTP1.1

    std::string host_;          ///< host about request
    std::string user_agent_;    ///< user agent about request

};


#endif // REQUEST_H
