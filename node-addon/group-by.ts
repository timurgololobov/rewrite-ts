import { arrayItem } from "./el.dto";

/**
 * Group items by field
 *
 * const items = [
 *  {id: 1, name: 'A', val: 1, smth: true},
 *  {id: 2, name: 'B', val: 2, ...},
 *  {id: 3, name: 'A', val: 3, ...},
 *  {id: 4, name: 'B', val: 4, ...},
 *  {id: 5, name: 'A', val: 5, ...}
 * ]
 *
 * groupBy(items, 'name', ['val', ...]);
 * // Result
 * [
 *   {
 *     groupField: 'name',
 *     groupValue: 'A',
 *
 *     // Items of the group
 *     items: [{id: 1, name: 'A', val: 1, , smth: true}, {id: 3, name: 'A', val: 3}, {id: 5, name: 'A', val: 5}],
 *
 *     // Values of first item
 *     id: 1,
 *     smth: true,
 *     ...
 *
 *     // Summed values
 *     val: 9,
 *     ...
 *   }, {
 *     groupField: 'name',
 *     groupValue: 'B',
 *     items: [{id: 2, name: 'B', val: 2}, {id: 4, name: 'B', val: 4}],
 *     id: 2,
 *     val: 6,
 *     ...
 *   }
 * ]
 *
 * @param items - Array<any> - items array
 * @param field - string - field name
 * @param sumFields - Array<string> - array with names of fields which should be summed
 * @returns Array<any>
 */

const itemsDef: arrayItem = {
  id: 0,
  group: "string",
  string: "string",
  number: 0,
  number2: 0,
  null: null,
  undefined: undefined,
  boolean: true,
};

type groupItem = {
  items: [];
  groupField: keyof typeof itemsDef;
  groupValue: string;
};

export function groupBy(
  items: arrayItem[],
  field: keyof typeof itemsDef,
  sumFields: string[] = []
): any[] {
  const groupsMap = {} as arrayItem;

  for (let i = 0; i < items.length; i++) {
    const item = items[i];
    const groupValue = String(item[field as keyof arrayItem]);
    let group = groupsMap[groupValue];

    if (!group) {
      group = {
        items: [],
        groupField: field,
        groupValue: groupValue,
        ...item,
      } as groupItem;
      groupsMap[groupValue] = group;
    }

    // Add values which should be summed
    for (let j = 0; j < sumFields.length; j++) {
      const fieldName = sumFields[j];
      const sumValue = item[fieldName];

      if (sumValue) {
        group[fieldName] += sumValue;
      }
    }

    group.items.push(item);
  }

  return Object.keys(groupsMap).map((key) => groupsMap[key]);
}
