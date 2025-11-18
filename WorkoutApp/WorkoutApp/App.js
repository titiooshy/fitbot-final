import * as React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createStackNavigator } from '@react-navigation/stack';
import HomeScreen from './screens/HomeScreen';
import QuestionnaireScreen from './screens/QuestionnaireScreen';
import WorkoutBuilderScreen from './screens/WorkoutBuilderScreen';

const Stack = createStackNavigator();

export default function App() {
  return (
    <NavigationContainer>
      <Stack.Navigator initialRouteName="Home">
        <Stack.Screen name="Home" component={HomeScreen} />
        <Stack.Screen name="Questionnaire" component={QuestionnaireScreen} />
        <Stack.Screen name="WorkoutBuilder" component={WorkoutBuilderScreen} />
      </Stack.Navigator>
    </NavigationContainer>
  );
}
