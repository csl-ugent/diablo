#include <diablosupport.h>

void
ManagerCallbackInstall (t_manager * manager, t_manager_callback_type type, t_int32 prior, t_manager_callback_function fun, void *data)
{
  t_manager_callback *search = NULL;
  t_manager_callback *cb = Malloc (sizeof (t_manager_callback));

  cb->cb_user_data = data;
  cb->cb_func = fun;
  cb->type = type;
  cb->prior = prior;

  switch (type)
  {
    case CB_NEW:
      {
        search = manager->new_;
        if ((!search) || (search->prior > prior))
        {
          cb->next = search;
          manager->new_ = cb;
          return;
        }
      }
      break;
    case CB_FREE:
      {
        search = manager->free;
        if ((!search) || (search->prior > prior))
        {
          cb->next = search;
          manager->free = cb;
          return;
        }
      }
      break;

    case CB_DUP:
      {
        search = manager->dup;
        if ((!search) || (search->prior > prior))
        {
          cb->next = search;
          manager->dup = cb;
          return;
        }
      }
    default:
      FATAL(("Unknown Callback type"));

  }

  while ((search->next) && (search->next->prior < prior))
  {
    search = search->next;
  }

  cb->next = search->next;
  search->next = cb;
}

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
