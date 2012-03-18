#ifndef	__CDATA_DEV_CLASS_H__
#define	__CDATA_DEV_CLASS_H__

#define	MAX_MINOR	15

struct cdata_dev {
	int 	minor;

	int 	(*connect)(struct cdata_dev *);
	int 	(*disconnect)(struct cdata_dev *);

	void	*private;
};

int cdata_device_register(struct cdata_dev *);
int cdata_device_unregister(struct cdata_dev *);

#endif
