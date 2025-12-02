import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createNativeStackNavigator } from '@react-navigation/native-stack';

// Import your screens
import HomeScreen from './src/screens/HomeScreen';
import QuestionnaireScreen from './src/screens/QuestionnaireScreen';
import WorkoutListScreen from './src/screens/WorkoutListScreen';

const Stack = createNativeStackNavigator();

export default function App() {
  return (
    <NavigationContainer>
      <Stack.Navigator initialRouteName="Home">
        <Stack.Screen 
          name="Home" 
          component={HomeScreen} 
          options={{ title: 'WorkoutApp' }}
        />
        <Stack.Screen 
          name="Questionnaire" 
          component={QuestionnaireScreen} 
          options={{ title: 'Questionnaire' }}
        />
        <Stack.Screen 
          name="Workouts" 
          component={WorkoutListScreen} 
          options={{ title: 'Workouts' }}
        />
      </Stack.Navigator>
    </NavigationContainer>
  );
}
