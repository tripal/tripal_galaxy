[![7.x-3.x Build Status](https://travis-ci.org/tripal/tripal_galaxy.svg?branch=7.x-1.x)](https://travis-ci.org/tripal/tripal_galaxy)
[![Documentation Status](https://readthedocs.org/projects/tripal_galaxy/badge/?version=latest)](https://tripal.readthedocs.io/en/latest/?badge=latest)

![alt tag](https://raw.githubusercontent.com/tripal/tripal/7.x-3.x/tripal/theme/images/tripal_logo.png)
![alt_tag](https://galaxyproject.org/images/galaxy-logos/galaxy_logo_25percent.png)


# Overview
The Tripal Galaxy module is designed to support integration of [Tripal](http://tripal.info) with [Galaxy](https://galaxyproject.org/). In the past, community databases have often provided analytical tools that come prepared with site-specific data. Examples include BLAST, CAP3, and InterProScan servers, for example. By integrating Tripal with Galaxy, the Tripal-based community database can offer more complicated analytical tools that support larger data sets using Galaxy as a backend. 

The Tripal Galaxy module provides more than just a “wrapper” for Galaxy. Site administrators can provide files to help end-users easily integrate data from the site within workflows. On Tripal v3 sites, user’s can create data collection containing data gleaned from the site which in turn can be used in Galaxy workflows. Quotas are provided to prevent users from overunning the storage space of the server and usage statistics help a site admin learn which workflows are most used and who are the biggest users.

# Installation
Please view the [latest online documentation](https://tripal-galaxy.readthedocs.io/en/latest/).

# Usage
Please view the [latest online documentation](https://tripal-galaxy.readthedocs.io/en/latest/).

# Unit Testing
Testing of this module can be performed using a pre-configured suite of Docker images and Docker compose. From within the Tripal Galaxy project directory, execute the following to perform testing:

```bash
# Fire up the docker images
docker-compose up -d

# Install blend4php
docker-compose exec app bash -c 'cd /var/www/html/sites/all/libraries; git clone https://github.com/galaxyproject/blend4php.git'

# Enable the Tripal Galaxy module
docker-compose exec app bash -c "cd /var/www/html; drush en -y tripal_galaxy"

# Start the Unit tests
docker-compose exec app bash -c 'cd /modules/tripal_galaxy; ./vendor/bin/phpunit'
```

