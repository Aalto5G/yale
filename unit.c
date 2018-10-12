#include "parser.h"

struct firstset_values singleton(yale_uint_t tkn)
{
  struct firstset_values values;
  struct firstset_value *value = malloc(1*sizeof(*value));
  value[0].is_bytes = 0;
  value[0].token = tkn;
  value[0].cbsz = 0;
  value[0].cbs = NULL;
  values.values = value;
  values.valuessz = 1;
  return values;
}

int main(int argc, char **argv)
{
  struct firstset_values vals1, vals2, vals3, vals4, vals5, vals6;
  size_t i;

  vals1 = singleton(1);
  vals2 = singleton(2);
  vals3 = singleton(3);
  vals4 = singleton(2);
  vals5 = singleton(1);
  vals6 = singleton(0);
  firstset2_update(NULL, &vals1, &vals2, 0, NULL);
  firstset2_update(NULL, &vals1, &vals3, 0, NULL);
  firstset2_update(NULL, &vals1, &vals4, 0, NULL);
  firstset2_update(NULL, &vals1, &vals5, 0, NULL);
  firstset2_update(NULL, &vals1, &vals6, 0, NULL);

  printf("%d tkns\n", (int)vals1.valuessz);
  for (i = 0; i < vals1.valuessz; i++)
  {
    printf("%d tkn\n", vals1.values[i].token);
  }
  firstset_values_deep_free(&vals1);
  firstset_values_deep_free(&vals2);
  firstset_values_deep_free(&vals3);
  firstset_values_deep_free(&vals4);
  firstset_values_deep_free(&vals5);
  firstset_values_deep_free(&vals6);
  return 0;
}
