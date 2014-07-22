Mate Devel
==========

Dev's Little Helpers for Drupal 7

## Drush commands

### Reset User Password

Change Users Pass : Sets a password to all users, ideally to be used after import a production
database to set the user password and access with an admin user to work in developments environments.

Usage :

```
drush nrp new_pass
```

By default drush nrp sets the password to 123

### Get remotes files

Download files from other server, ideally to get production files for locals environments.

Usage:

```
drush grf http://www.myproductionsite.com
```

There is more parameters for networks with proxies.

### Delete all content

Deletes all content for a given content type o for all

Usage:

```
drush dc type
```

### Set module schema version

Sets the version number of a module schema to an specific version, ideally used to set
the version when you are creating a hook_update and need to test it and rollback several
times.

Usage :

```
drush ssv my_module 7XXX
```

### Get Production Database

Get a database of production is usually a pain, to get a backup we provide a page
callback that receives a secret key, that is set with variable_set called mate_devel_key,
and this callback makes a dump of the database using backup_migrate module and does an echo
of the dump to send it back as response of the request.

Set the mate_devel_key

```
drush vset mate_devel_key 'SUPERKEY'
```

Whit the module enabled you can do in the console

```
curl -sS http://www.mysite.com/mate_devel/backup/SUPERKEY > backup.sql
```

Be aware of the security concerns of this features if you need to use it.
For large database this module is not a good fit, don't use it.


### More Ideas

Are welcome :)

