/*------------------------------------------------------
  Includes :
  ------------------------------------------------------*/

/* Standard includes */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#if UNIX
#include <signal.h>
#include <sys/time.h>
#endif

/*------------------------------------------------------
  Defines :
  ------------------------------------------------------*/

#define NO_CORREL_ID    0       /* CorrelationId par défaut quand la nvl n'est pas dispo */

/* Longeur maximale des prefixes des traces */
#define DBG_TAB_MAX        40

/* Longeur maximale des prefixes des traces */
#define DBG_STR_MAX        20

/* Modes */
#define DBG_MODE_DATE      0x00000001L
#define DBG_MODE_TIME      0x00000002L
#define DBG_MODE_FLUSH     0x00000004L
#define DBG_MODE_COUNTER   0x00000008L
#define DBG_MODE_TIMER     0x00000010L
#define DBG_MODE_IDENT     0x00000020L

/* Niveaux */
#define DBG_LEVEL_DEFAULT  0x00000001L
#define DBG_LEVEL_VALUE    0x00000002L
#define DBG_LEVEL_DUMP     0x00000004L
#define DBG_LEVEL_DEBUG    0x00000008L
#define DBG_LEVEL_ENTER    0x00000010L
#define DBG_LEVEL_RETURN   0x00000020L
#define DBG_LEVEL_FUNCTION 0x00000030L
#define DBG_LEVEL_ADMIN    0x00000040L

#define DBG_LEVEL_ERROR    0x00000100L
#define DBG_LEVEL_WARNING  0x00000200L

/* Niveau utilisateur */
#define DBG_LEVEL_USER1    0x00010000L
#define DBG_LEVEL_USER2    0x00020000L
#define DBG_LEVEL_USER3    0x00040000L
#define DBG_LEVEL_USER4    0x00080000L
#define DBG_LEVEL_USER5    0x00100000L
#define DBG_LEVEL_USER6    0x00200000L
#define DBG_LEVEL_USER7    0x00400000L
#define DBG_LEVEL_USER8    0x00800000L
#define DBG_LEVEL_USER9    0x01000000L
#define DBG_LEVEL_USER10   0x02000000L
#define DBG_LEVEL_USER11   0x04000000L
#define DBG_LEVEL_USER12   0x08000000L
#define DBG_LEVEL_USER13   0x10000000L
#define DBG_LEVEL_USER14   0x20000000L
#define DBG_LEVEL_USER15   0x40000000L
#define DBG_LEVEL_USER16   0x80000000L

#define DBG_LEVEL_ALL      0xFFFFFFFFL

/* Niveau simplifies */
#define DBG_ENT            DBG_LEVEL_ENTER
#define DBG_RET            DBG_LEVEL_RETURN
#define DBG_VAL            DBG_LEVEL_VALUE
#define DBG_DMP            DBG_LEVEL_DUMP
#define DBG_ERR            DBG_LEVEL_ERROR
#define DBG_WAR            DBG_LEVEL_WARNING

/* Actions */
#define DBG_ACTION_OPEN        0x00000001L
#define DBG_ACTION_CLOSE       0x00000002L
#define DBG_ACTION_FLUSH       0x00000004L
#define DBG_ACTION_CLEAR       0x00000008L
#define DBG_ACTION_ROLL        0x00000010L
#define DBG_ACTION_STOP        0x00000020L
#define DBG_ACTION_RELOAD_CFG  0x00000040L

/* Textes des niveaux */
#define DBG_STR_LOG     "LOG  "
#define DBG_STR_ENTER   "ENT  "
#define DBG_STR_RETURN  "RET  "
#define DBG_STR_DUMP    "DMP  "
#define DBG_STR_DEBUG   "BUG  "
#define DBG_STR_ERROR   "ERR  "
#define DBG_STR_WARNING "WAR  "
#define DBG_STR_DEFAULT "???  "
#define DBG_STR_USER    "USR  "

/* Options de compilation */
#define DBG_OPTION_DUMP_OFFSET
#define DBG_OPTION_DATETIME

/* taille max d'un fichier */
#define MAX_FILENAME_LEN  255

/* taille max d'un nom de module */
#define MAX_MODULE_NAME_LEN   50

#define SIZEOF_LONG         (sizeof (long) * 8)
#define NB_LONG_TO_MAKE_256 ((int)(256 / SIZEOF_LONG))  /* NB_LONG_TO_MAKE_256 * SIZEOF_LONG = 256 => utilisé pour codé le filtre a correlationId */

/*------------------------------------------------------
  Functions :
  ------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif

  void DBG_mode(unsigned long option, int state);
  void DBG_level(unsigned long option, int state);
  void DBG_action(unsigned long action);
  int DBG_test_and_store_level(unsigned long level);
  void DBG_string(unsigned long level, const char *string);
  void DBG_filename(char *module_name);
  int DBG_open(char *module_name);
  int DBG_init(char *module_name, int handler);
  int DBG_close(void);
  void DBG_size(long size);
  void DBG_put(unsigned int correlationId, char *data);
  void DBG_trace(unsigned int correlationId, const char *format, ...);
  void DBG_enter(unsigned int correlationId, char *function);
  void DBG_return(unsigned int correlationId, char *function);
  void DBG_trame(unsigned int correlationId, char *name, unsigned char *data,
    int length);

#ifdef __cplusplus
}
#endif

/*------------------------------------------------------
  Defines :
  ------------------------------------------------------*/

/* Niveaux */
#define DBG_LEVEL_MYSQL    DBG_LEVEL_USER1  /*!< niveau pour les traces concernant le protocol IPAP */
#define DBG_LEVEL_ALARM    DBG_LEVEL_USER10 /*!< niveau pour les traces concernant le FAX */
#define DBG_LEVEL_ASTERISK DBG_LEVEL_USER11 /*!< niveau pour les traces concernant le TCP */
#define DBG_LEVEL_TCP      DBG_LEVEL_USER12 /*!< niveau pour les traces concernant la lib sgn de gestion de flux  */
#define DBG_LEVEL_QOS      DBG_LEVEL_USER13 /*!< niveau pour les traces concernant la lib QOS   */

/* Niveau simplifies */
#define DBG_MYSQL          DBG_LEVEL_MYSQL
#define DBG_ALARM          DBG_LEVEL_ALARM
#define DBG_ASTERISK       DBG_LEVEL_ASTERISK
#define DBG_TCP            DBG_LEVEL_TCP
#define DBG_QOS            DBG_LEVEL_QOS

/*------------------------------------------------------
  Macros :
  ------------------------------------------------------*/

/*!
 * \brief Ouvre le fichier de log "module_name.log" et fait les initialisations necessaires
 *
 * \param module_name nom du module au format "SVI_module"
 **/
#define DBG_OPEN(module_name)     \
  DBG_string(DBG_LEVEL_MYSQL    ,"MYSQL");  \
  DBG_string(DBG_LEVEL_ALARM    ,"ALARM");  \
  DBG_string(DBG_LEVEL_ADMIN    ,"ADMIN");  \
  DBG_string(DBG_LEVEL_ASTERISK ,"ASTERISK");    \
  DBG_string(DBG_LEVEL_TCP      ,"TCP");    \
  DBG_string(DBG_LEVEL_QOS      ,"QOS");    \
  DBG_init ((char*)module_name, 0);          \
  DBG_mode(DBG_MODE_IDENT, 1);            \
  DBG_mode(DBG_MODE_TIMER, 1);            \
  DBG_level (DBG_LEVEL_ERROR, 1);

/*!
 * \brief Ferme l'accès au systeme de log
 **/
#define DBG_CLOSE()   \
  DBG_close()

/*!
 * \brief Permet d'envoyé une trace formaté à la printf
 *
 * \param level niveau de la trace (utilisé les defines DBG_LEVEL_*)
 * \param Y => (correlationId, format_string, ...)
 * \param correlationId numéro de la nvl courrante ou NO_CORREL_ID si pas de nvl valide
 * \param format_string chaine de format a la printf
 * \param ... arguments à formaté dans la chaine
 *
 * \warning Pour afficher une simple chaine, il faut utilisé DBG_PUT()
 * \warning Tout le contenu de Y doit etre parenthèsé. 
 **/
#define DBG_TRACE(level, Y)   \
  if (DBG_test_and_store_level (level))   \
  {                                       \
    DBG_trace Y;   \
  }

/*!
 * \brief Permet d'envoyer une trace sous forme de simple chaine de caractère 
 *
 * \param level niveau de la trace (utilisé les defines DBG_LEVEL_*)
 * \param correlationId numéro de la nvl courrante ou NO_CORREL_ID si pas de nvl valide
 * \param message message de la trace
 **/
#define DBG_PUT(level, correlationId, message)           \
  if (DBG_test_and_store_level (level))   \
  {                                       \
    DBG_put(correlationId, (char*)message);                \
  }

/*!
 * \brief Envoie le nom d'un pointeur et créé un dump de son contenu
 *
 * \param level niveau de la trace (utilisé les defines DBG_LEVEL_*)
 * \param correlationId numéro de la nvl courrante ou NO_CORREL_ID si pas de nvl valide
 * \param name Nom de la donnée à dumper
 * \param pointer pointeur vers la structure de donnée à dumper
 * \param lenght longueur de la structure de donnée a dumper
 * 
 * \see DBG_TRAME()
 **/
#define DBG_DUMP(level, correlationId, name, pointer, lenght) \
  if (DBG_test_and_store_level (level))   \
  {                                       \
    DBG_trame(correlationId, (char*)name, (unsigned char*)pointer, lenght);           \
  }

/*!
 * \brief Envoie le nom d'un pointeur sur les data d'une trame et créé un dump de son contenu
 *
 * \see Voir DBG_DUMP() pour plus de détails.
 **/
#define DBG_TRAME(level, correlationId, name, pointer, lenght) \
  if (DBG_test_and_store_level (level))   \
  {                                       \
    DBG_trame(correlationId, (char*)name, (unsigned char*)pointer, lenght);      \
  }

/*!
 * \brief Informe de l'entrée dans une fonction
 *
 * \param correlationId numéro de la nvl courrante ou NO_CORREL_ID si pas de nvl valide
 * \param message Nom de la fonction ou de la section où l'on vient d'entrer
 *
 * \remarks le niveau utilisé est automatiquement mis à #DBG_LEVEL_ENTER.
 **/
#define DBG_ENTER(correlationId, message)                \
  DBG_enter(correlationId, (char *)message)

/*!
 * \brief Informe de la sortie d'une fonction
 *
 * \param correlationId numéro de la nvl courrante ou NO_CORREL_ID si pas de nvl valide
 * \param message Nom de la fonction ou de la section d'où l'on vient de sortir
 *
 * \remarks le niveau utilisé est automatiquement mis à #DBG_LEVEL_RETURN. 
 **/
#define DBG_RETURN(correlationId, message)               \
  DBG_return(correlationId, (char *)message)

/*!
 * \brief Informe sur le nom du fichier source et la position dans ce fichier.
 *
 * \param level niveau de la trace (utilisé les defines DBG_LEVEL_*)
 * \param correlationId numéro de la nvl courrante ou NO_CORREL_ID si pas de nvl valide
 **/
#define DBG_POSITION(correlationId, level)               \
  DBG_TRACE(level, (correlationId, "%s %s", __FILE__, __LINE__))

/*!
 * \brief Affiche une trace de warning
 *
 * \param correlationId numéro de la nvl courrante ou NO_CORREL_ID si pas de nvl valide
 * \param message message de la trace
 *
 * \remarks le niveau utilisé est automatiquement mis à #DBG_LEVEL_WARNING.
 **/
#define DBG_WARNING(correlationId, message)              \
  DBG_PUT(DBG_LEVEL_WARNING, correlationId, message)

/*!
 * \brief Affiche une trace d'erreur
 *
 * \param correlationId numéro de la nvl courrante ou NO_CORREL_ID si pas de nvl valide
 * \param message message de la trace
 *
 * \remarks le niveau utilisé est automatiquement mis à #DBG_LEVEL_ERROR.
 **/
#define DBG_ERROR(correlationId, message)                \
  DBG_PUT(DBG_LEVEL_ERROR, correlationId, message)

/*!
 * \brief Effectue un assert sur une condition et log la position en cas d'erreur
 *
 * \param correlationId numéro de la nvl courrante ou NO_CORREL_ID si pas de nvl valide
 * \param condition condition qui doit etre rempli pour evité l'assert
 *
 * \remarks le niveau utilisé est automatiquement mis à #DBG_LEVEL_ERROR.
 **/
#define DBG_ASSERT(correlationId, condition)             \
  if(!(condition))                        \
  {                                       \
		DBG_TRACE(DBG_LEVEL_ERROR, (correlationId, "Assert exit : file %s at line %s !" __FILE__, __LINE__ )); \
    assert(condition);                    \
  }

/*!
 * \brief Active/désactive un niveau de trace
 *
 * \param level niveau de la trace à changer (utilisé les defines DBG_LEVEL_*)
 * \param enable 1 = activé / 0 = désactivé
 **/
#define DBG_LEVEL(level, enable) \
  DBG_level(level, enable)

/*!
 * \brief Active/désactive un mode
 *
 * \param valeur du mode à changer (utilisé les defines DBG_MODE_*)
 * \param enable 1 = activé / 0 = désactivé
 **/
#define DBG_MODE(mode, enable) \
  DBG_mode(mode, enable)

/*!
 * \brief Execute une action
 *
 * \param valeur de l'action à executer (utilisé les defines DBG_ACTION_*)
 **/
#define DBG_ACTION(command)   \
  DBG_action(command)


/*------------------------------------------------------
  Variables :
  ------------------------------------------------------*/


/* Buffers */
static char DBG_buffer[10000];
static char DBG_buffer2[10000];
static char DBG_module_name[MAX_MODULE_NAME_LEN + 1];
static char DBG_filename_buffer[MAX_FILENAME_LEN + 1];
static char DBG_str[16][DBG_STR_MAX] = {
  DBG_STR_USER,
  DBG_STR_USER,
  DBG_STR_USER,
  DBG_STR_USER,
  DBG_STR_USER,
  DBG_STR_USER,
  DBG_STR_USER,
  DBG_STR_USER,
  DBG_STR_USER,
  DBG_STR_USER,
  DBG_STR_USER,
  DBG_STR_USER,
};
static int DBG_strlen[16] = {
  4,
  4,
  4,
  4,
  4,
  4,
  4,
  4,
  4,
  4,
  4,
  4,
};

/* Pointeur de fichier */
static FILE *DBG_file = NULL;

/* Modes et niveaux */
unsigned long DBG_modes = DBG_MODE_DATE | DBG_MODE_TIME | DBG_MODE_IDENT;
unsigned long DBG_levels = 0;
unsigned long DBG_levels2test;

/* Structure de temps */
static struct tm *DBG_time_struct;
static unsigned long DBG_counter = 0;
static unsigned long DBG_lenght = 10000000;
static unsigned long DBG_position = 0;
static unsigned long DBG_nb_max_file = 4;
static unsigned long DBG_num_current_file = 0;
static int DBG_tab = 0;
//static int              DBG_modulo = 16;


/*------------------------------------------------------
  DBG_mode
  
  Fonction pour changer les modes du systeme de traces.
  ------------------------------------------------------*/
void DBG_mode(unsigned long option, int state)
{
  if (state)
  {
    DBG_TRACE(DBG_LEVEL_ADMIN, (NO_CORREL_ID, "Set modes = %08X.", option));
  }
  else
  {
    DBG_TRACE(DBG_LEVEL_ADMIN, (NO_CORREL_ID, "Clear modes = %08X.", option));
  }

  if (state)
  {
    DBG_modes |= option;
  }
  else
  {
    DBG_modes &= ~option;
  }

  DBG_TRACE(DBG_LEVEL_ADMIN, (NO_CORREL_ID, "Modes = %08X.", DBG_modes));
}


/*------------------------------------------------------
  DBG_level

  Changer les niveaux de traces du systeme de traces.
  ------------------------------------------------------*/
void DBG_level(unsigned long level, int state)
{
  if (state)
  {
    DBG_TRACE(DBG_LEVEL_ADMIN, (NO_CORREL_ID, "Set levels = %08X.", level));
  }
  else
  {
    DBG_TRACE(DBG_LEVEL_ADMIN, (NO_CORREL_ID, "Clear levels = %08X.", level));
  }

  if (state)
  {
    DBG_levels |= level;
    DBG_levels &= 0xFFFFFFFF;
  }
  else
  {
    DBG_levels &= ~level;
  }

  DBG_TRACE(DBG_LEVEL_ADMIN, (NO_CORREL_ID, "Levels = %08X.", DBG_level));
}


/*------------------------------------------------------
  DBG_string

  Definir les chaines prefixe des traces.
  ------------------------------------------------------*/
void DBG_string(unsigned long level, const char *string)
{
  int lenght;
  int index;

  lenght = strlen(string);

  if (lenght > (DBG_STR_MAX - 1))
    return;

  for (index = 0; index < 16; index++)
  {
    if (level & 1 << (index + 16))
    {
      strcpy(DBG_str[index], string);
      DBG_str[index][lenght] = ' ';
      DBG_str[index][lenght + 1] = '\0';
      DBG_strlen[index] = lenght + 1;
    }
  }
}

/*------------------------------------------------------
  DBG_exit

  Fonction abonnee a la fermeture du processus.
  ------------------------------------------------------*/
void DBG_exit(int code)
{
  DBG_TRACE(DBG_LEVEL_ERROR,
    (NO_CORREL_ID,
      "Process terminated, signal %s received.", _sys_siglist[code]));
  DBG_close();

  /* rend le signal */
  raise(code);
}


/*------------------------------------------------------
  DBG_filename
  
  Permettre de definir un nom de fichier par defaut pour 
  une ouverture differee.
  ------------------------------------------------------*/
void DBG_filename(char *module_name)
{
  int ret = -1;
  const char *path = "/tmp";

  {
    strncpy(DBG_module_name, module_name, MAX_MODULE_NAME_LEN);
    if (path)
    {
      /* Fichier de debug */
      ret =
        snprintf(DBG_filename_buffer, MAX_FILENAME_LEN, "%s/traces_%s-%ld.txt",
        path, module_name, DBG_num_current_file);
    }

    if (ret == -1)              /* test si nom tronqué par snprintf ou path incorrect */
    {
      *DBG_filename_buffer = 0;
    }
  }
}


/*------------------------------------------------------
  DBG_open

  Ouvrir le fichier de traces.
 *------------------------------------------------------*/
int DBG_open(char *module_name)
{
  if (DBG_file)
    DBG_close();

  if (module_name != NULL)
    DBG_filename(module_name);

  if (*DBG_filename_buffer != 0)
    DBG_file = fopen(DBG_filename_buffer, "w");
  if (DBG_file)
  {
    return 1;
  }

  return 0;
}


/*------------------------------------------------------
  DBG_init

  initialise les traces
 *------------------------------------------------------*/
int DBG_init(char *module_name, int handler)
{
  struct sigaction action;

  if (DBG_open(module_name))
  {
    DBG_tab = 0;
    DBG_PUT(DBG_LEVEL_DEFAULT, 0, "Start.");
    if (handler)
    {
      action.sa_handler = DBG_exit;
      sigemptyset(&action.sa_mask);
      action.sa_flags = SA_RESETHAND;

      sigaction(SIGHUP, &action, NULL);
      sigaction(SIGINT, &action, NULL);
      sigaction(SIGQUIT, &action, NULL);
      sigaction(SIGABRT, &action, NULL);
      sigaction(SIGILL, &action, NULL);
      sigaction(SIGTRAP, &action, NULL);
      sigaction(SIGSYS, &action, NULL);
      sigaction(SIGFPE, &action, NULL);
      sigaction(SIGBUS, &action, NULL);
      sigaction(SIGTERM, &action, NULL);
      sigaction(SIGSEGV, &action, NULL);
    }

    return 1;
  }

  return 0;
}


/*------------------------------------------------------
  DBG_close

  Fermer le fichier des tarces.
  ------------------------------------------------------*/
int DBG_close(void)
{
  if (DBG_file != NULL)
  {
    fclose(DBG_file);
    DBG_file = NULL;
    return 0;
  }
  return 1;
}


/*------------------------------------------------------
  DBG_size
  
  Definir la taille du fichier tourant.
  ------------------------------------------------------*/
void DBG_size(long size)
{
  DBG_lenght = size;
}


/*------------------------------------------------------
  DBG_action
  
  Executer une action du systeme de trace.
  ------------------------------------------------------*/
void DBG_action(unsigned long action)
{
  DBG_TRACE(DBG_LEVEL_ADMIN, (NO_CORREL_ID, "Action = 0x%08X.", action));

  if (action & DBG_ACTION_OPEN)
  {
    DBG_open(NULL);
  }
  else if (action & DBG_ACTION_ROLL)
  {
    /* Force a faire tourner le fichier */
    DBG_num_current_file++;
    if (DBG_num_current_file == DBG_nb_max_file)
    {
      DBG_num_current_file = 0;
    }
    DBG_open(DBG_module_name);
  }
  else if (action & DBG_ACTION_CLOSE)
  {
    DBG_close();
  }
  else if (action & DBG_ACTION_FLUSH)
  {
    if (DBG_file != NULL)
      fflush(DBG_file);
  }
}

/*------------------------------------------------------
  DBG_test_and_store_level

  Test si le level est ok et stocke la valeur du level pour
  la suite.
  ------------------------------------------------------*/

int DBG_test_and_store_level(unsigned long level)
{
  int ok = 1;

  if (level != DBG_LEVEL_ERROR)
  {
    if (!(DBG_levels & level))
    {
      ok = 0;
    }
  }
  DBG_levels2test = level;
  return ok;
}

/*------------------------------------------------------
  DBG_header
  
  Formater l'entete de la tarce en focntion des modes
  actives.
  ------------------------------------------------------*/
char *DBG_header(unsigned int correlationId)
{
  char *pointer = DBG_buffer;
  //char* keyword;

  static long datetime;
#ifdef DBG_OPTION_DATETIME
  static long old_datetime;
#endif

#ifdef UNIX
  struct timeval tp;
#endif

  if (DBG_modes & (DBG_MODE_DATE | DBG_MODE_TIME))
  {
    time(&datetime);
#ifdef DBG_OPTION_DATETIME
    if (old_datetime != datetime)
    {
      DBG_time_struct = localtime(&datetime);
      old_datetime = datetime;
    }
#else
    DBG_time_struct = localtime((void *)&datetime);
#endif

    if (DBG_modes & DBG_MODE_DATE)
    {
      sprintf(pointer, "%02d/%02d/%02d ",
        DBG_time_struct->tm_mday,
        DBG_time_struct->tm_mon + 1, DBG_time_struct->tm_year % 100);

      pointer += 9;
    }

    if (DBG_modes & DBG_MODE_TIME)
    {
      sprintf(pointer, "%02d:%02d:%02d ",
        DBG_time_struct->tm_hour,
        DBG_time_struct->tm_min, DBG_time_struct->tm_sec);
      pointer += 9;
    }

    if (DBG_modes & DBG_MODE_TIMER)
    {
#ifdef UNIX
#ifdef OS_SOLARIS
      gettimeofday(&tp, NULL);
#else
      gettimeofday(&tp);
#endif

#ifdef OS_SOLARIS
      sprintf(pointer, "%06d ", tp.tv_usec);
      pointer += 7;
#else
      sprintf(pointer, "%02d ", (tp.tv_usec / 10000));
      pointer += 3;
#endif
#endif
    }
  }

  if (DBG_modes & DBG_MODE_COUNTER)
  {
    DBG_counter++;
    sprintf(pointer, "%8ld ", DBG_counter);
    pointer += 9;
  }

  if (correlationId != NO_CORREL_ID)
  {
    sprintf(pointer, "0x%04X|%04d ", correlationId, correlationId);
    pointer += 12;
  }
  else
  {
    strcpy(pointer, "  --none--  ");
    pointer += 12;
  }


/*  DBG_levels2test &=0xFFFF0FFF; */

  /*if (DBG_levels2test)
     {
     int index;
     index = DBG_levels2test & 0xFFFF0FFF >> 24;
     strcpy (pointer, DBG_str[index]);
     pointer += 10;
     } 
     else */
  {
    strcpy(pointer, "  UNKNOWN ");
    pointer += 10;
  }

  switch (DBG_levels2test)
  {
    case DBG_LEVEL_ADMIN:
    case DBG_LEVEL_WARNING:
    case DBG_LEVEL_ERROR:
      break;
    default:
      if (DBG_modes & DBG_MODE_IDENT)
      {
        memset(pointer, ' ', DBG_tab);
        pointer += DBG_tab;

        if (DBG_tab == DBG_TAB_MAX)
        {
          *pointer = '~';
          pointer++;
        }
      }
  }

  return pointer;
}


/*------------------------------------------------------
  DBG_end
  
  Formater la fin des traces et provoque l'ecriture sur
  le disque.
  ------------------------------------------------------*/
void DBG_end(char *pointer)
{
  long index;

  *pointer++ = '\n';
  *pointer = 0x00;

  /* Impression sur la console */
  if (DBG_levels2test & DBG_LEVEL_ERROR)
  {
    fprintf(stderr, "%s> %s", DBG_module_name, DBG_buffer);
  }

  index = fputs(DBG_buffer, DBG_file);

  if (DBG_lenght > 0)
  {
    if (index != EOF)
    {
      DBG_position += index;
    }
    if (DBG_position > DBG_lenght || index == EOF)
    {
      DBG_position = 0;
      DBG_num_current_file++;
      if (DBG_num_current_file == DBG_nb_max_file)
      {
        DBG_num_current_file = 0;
      }
      fputs("--- End of file - Switching to next file ---", DBG_file);
      DBG_open(DBG_module_name);
    }
  }

  if (DBG_modes & DBG_MODE_FLUSH)
    fflush(DBG_file);
}


/*------------------------------------------------------
  DBG_put
  
  Tracer une chaine de caracteres dans le systeme de 
  traces avec un niveau specifie.
  ------------------------------------------------------*/
void DBG_put(unsigned int correlationId, char *data)
{
  char *pointer;
  int lenght;

  if (DBG_file == NULL)
    return;

  pointer = DBG_header(correlationId);

  lenght = strlen(data);

  if (lenght > 0)
  {
    strcpy(pointer, data);
    if (data[lenght - 1] < 32)
      pointer += (lenght - 1);
    else
      pointer += lenght;
  }

  DBG_end(pointer);
}

/*------------------------------------------------------
  DBG_trace

  affiche une chaine à la printf
  ------------------------------------------------------*/

void DBG_trace(unsigned int correlationId, const char *format, ...)
{
  va_list list;
  char *message = DBG_buffer2;

  va_start(list, format);
  vsprintf(message, format, list);
  va_end(list);

  DBG_put(correlationId, message);

}

/*------------------------------------------------------
  DBG_enter
  
  Tracer une entree dans une fonction.
  ------------------------------------------------------*/
void DBG_enter(unsigned int correlationId, char *function)
{
  if (DBG_levels & DBG_LEVEL_ENTER)
  {
    if (function != NULL)
    {
      DBG_levels2test = DBG_LEVEL_ENTER;
      DBG_put(correlationId, function);
    }
  }

  if (DBG_modes & DBG_MODE_IDENT)
  {
    if (DBG_tab < DBG_TAB_MAX)
      DBG_tab++;
  }
}


/*------------------------------------------------------
  DBG_return
  
  Tracer une sortie dans une fonction.
  ------------------------------------------------------*/
void DBG_return(unsigned int correlationId, char *function)
{
  if (DBG_levels & DBG_LEVEL_RETURN)
  {
    if (function != NULL)
    {
      DBG_levels2test = DBG_LEVEL_RETURN;
      DBG_put(correlationId, function);
    }
  }

  if (DBG_modes & DBG_MODE_IDENT)
  {
    if (DBG_tab > 0)
    {
      DBG_tab--;
    }
  }
}


/*------------------------------------------------------
  DBG_trame
  
  Tracer un buffer de caracteres. 
  ------------------------------------------------------*/
void DBG_trame(unsigned int correlationId, char *name, unsigned char *data,
  int length)
{
  int index;
  char *pointer;

  pointer = DBG_header(correlationId);

  if (name == NULL)
    sprintf(pointer, "(%d) :", length);
  else
    sprintf(pointer, "%s (%d) :", name, length);

  pointer = DBG_buffer + strlen(DBG_buffer);

  for (index = 0; index < length; index++)
  {
    sprintf(pointer, "%02X", data[index]);
    pointer += 2;

    /* if((((index+1) % DBG_modulo)==0) || (index+1==length)) */
    /* if(((int)pointer>(int)&DBG_buffer[70]) || (index+1==length)) */
    if (index + 1 == length)
    {
      DBG_levels2test = DBG_LEVEL_DUMP;
      DBG_end(pointer);

      memset(DBG_buffer, ' ', 8);
      pointer = DBG_buffer + 8;
    }
  }

  return;
}
