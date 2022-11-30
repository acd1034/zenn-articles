import Control.Exception
import Text.Read

-- parseExpr :: String -> Maybe Int
-- parseExpr s = do
--   let [tok0, tok1, tok2] = words s
--   n <- readMaybe tok0 :: Maybe Int
--   m <- readMaybe tok2 :: Maybe Int
--   case tok1 of
--     "+" -> Just $ n + m
--     "-" -> Just $ n - m
--     "*" -> Just $ n * m
--     "/" -> Just $ n `div` m
--     _ -> Nothing

parseExpr :: String -> Maybe Int
parseExpr s =
  let [tok0, tok1, tok2] = words s in
    (readMaybe tok0 :: Maybe Int) >>= \n ->
      (readMaybe tok2 :: Maybe Int) >>= \m ->
        case tok1 of
          "+" -> Just $ n + m
          "-" -> Just $ n - m
          "*" -> Just $ n * m
          "/" -> Just $ n `div` m
          _ -> Nothing

-- main :: IO ()
-- main = do
--   assert (parseExpr "1 + 2" == (Just $ 1 + 2)) $
--     assert (parseExpr "478 - 234" == (Just $ 478 - 234)) $
--     assert (parseExpr "15 * 56" == (Just $ 15 * 56)) $
--     assert (parseExpr "98 / 12" == (Just $ 98 `div` 12)) $
--     return ()

main :: IO ()
main = do
  assert (map parseExpr ["1 + 2", "478 - 234", "15 * 56", "98 / 12"]
    == [Just $ 1 + 2, Just $ 478 - 234, Just $ 15 * 56, Just $ 98 / 12]) $
    return ()
