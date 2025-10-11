#include <vector>
#include <string>
#include <iostream>

struct Location
{
    std::string path;
    std::string root;
};

Location* findBestMatch(std::vector<Location>& locations, const std::string uri)
{
    int index = -1;

    for (size_t i = 0; i < locations.size(); i++)
    {
        std::string loc = locations[i].path;

        if (uri.compare(0, loc.length(), loc) == 0
            && (uri.length() == loc.length() || uri[loc.length()] == '/' || loc == "/"))
        {
            if (index != -1 && locations[index].path.length() <  locations[i].path.length())
                index = i;
            else if (index == -1)
                index = i;
        }
    }
    if (index == -1)
        return NULL;
    else
        return &locations[index];
}

int main()
{
    std::vector<Location> locations;
    locations.push_back((Location){"/", "/var/www/html"});
    locations.push_back((Location){"/upload/images/jdb", "/var/www/img"});
    locations.push_back((Location){"/upload", "/var/www/uploads"});
    locations.push_back((Location){"/cgi-bin/upload/jdbvdf/vfdhkvbdf/dfvdfv", "/var/www/cgi-bin"});

    std::string uri = "/upload/images/jdb";
    Location* match = findBestMatch(locations, uri);

    std::cout << "Best match: " << match->path << std::endl;
    std::cout << "Root: " << match->root << std::endl;

    return 0;
}
// uri = /upload/vjfvfv

// loc = /
// loc = /upload/images/jdb