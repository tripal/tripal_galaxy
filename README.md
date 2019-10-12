[![7.x-3.x Build Status](https://travis-ci.org/tripal/tripal_galaxy.svg?branch=7.x-1.x)](https://travis-ci.org/tripal/tripal_galaxy)
[![Documentation Status](https://readthedocs.org/projects/tripal_galaxy/badge/?version=latest)](https://tripal-galaxy.readthedocs.io/en/latest/?badge=latest)
[![Maintainability](https://api.codeclimate.com/v1/badges/d96b8ae3505c31c48d31/maintainability)](https://codeclimate.com/github/tripal/tripal_galaxy/maintainability)
[![Test Coverage](https://api.codeclimate.com/v1/badges/d96b8ae3505c31c48d31/test_coverage)](https://codeclimate.com/github/tripal/tripal_galaxy/test_coverage)
[![DOI](https://zenodo.org/badge/56126128.svg)](https://zenodo.org/badge/latestdoi/56126128)

![alt tag](https://raw.githubusercontent.com/tripal/tripal/7.x-3.x/tripal/theme/images/tripal_logo.png)
![alt_tag](https://galaxyproject.org/images/galaxy-logos/galaxy_logo_25percent.png)


# Overview
The Tripal Galaxy module is designed to support integration of [Tripal](http://tripal.info) with [Galaxy](https://galaxyproject.org/). It uses the [blend4php](https://github.com/galaxyproject/blend4php) library. In the past, community databases have often provided analytical tools that come prepared with site-specific data. Examples include BLAST, CAP3, and InterProScan servers, for example. By integrating Tripal with Galaxy, the Tripal-based community database can offer more complicated analytical tools that support larger data sets using Galaxy as a backend. 

The Tripal Galaxy module provides more than just a “wrapper” for Galaxy. Site administrators can provide files to help end-users easily integrate data from the site within workflows. On Tripal v3 sites, user’s can create data collection containing data gleaned from the site which in turn can be used in Galaxy workflows. Quotas are provided to prevent users from overunning the storage space of the server and usage statistics help a site admin learn which workflows are most used and who are the biggest users.

# Compatibility
The Tripal Galaxy module is currently only compatible with Galaxy release v19.01.  

# How does it work!
## Option 1: Create a web form for a workflow
1. Identify users who need analyatical workflows within the research community that your Tripal site supports.
2. Determine if you have access to a Galaxy server that has the space and computational resources to support these analyses. The [Use Galaxy server](https://usegalaxy.org/) is an option but is a globally shared resources.
3. Create the workflow using the Galaxy web interface, or consider trying some pre-existing workflows housed at the [Staton Lab Galaxy Workflows repository](https://github.com/statonlab/galaxy-workflows).   You can edit those workflows as needed to meet your needs.
4. Use the [Galaxy Aurora Tools](https://github.com/statonlab/aurora-galaxy-tools) in your workflow to provide visualization of results. This makes it easy for end-users to understand results.
5. Use the Tripal Galaxy module to create a webform within your Tripal site to allow users to execute the workflow.  User's need not interact with Galaxy at all.
6. Set users quotas using Tripal to ensure you do not overun the storage space on your server.
7. Create a link to the workflow in an accessible location so end-users can run the workflow.

## Option 2: Use a Galaxy workflow to power an application
1. Create a Galaxy workflow that will execute the analysis you need for your application using the Galaxy web interface.
2. Create a web front-end for an application to run on your Tripal site.  Use the Tripal API, Drupal API, and other visualization libraries you need.
3. Use the Tripal Galaxy API to write code that submits the workflow behind-the-scenes, checks the status and retrieves results.

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
